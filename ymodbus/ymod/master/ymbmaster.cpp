/**
* ymodbus
* Copyright © 2019-2019 liuyongqing<lyqdy1@163.com>
* v1.0.1 2019.05.04
*/
#include "ymod/master/ymbmaster.h"
#include "ymod/ymbtask.h"

#include "ymod/ymbnet.h"
#include "ymod/ymbrtu.h"
#include "ymod/ymbascii.h"

#include "ymod/master/ytcpconnect.h"
#include "ymod/master/yserconnect.h"
#include "ymod/master/yudpconnect.h"

#include "ymbopts.h"

#include <queue>
#include <mutex>
#include <condition_variable>
#include <cstring>

namespace YModbus {

namespace {

struct Request
{
	Request(MsgInf &i) : err(0), inf(i) {}
	int err;	//error, api or logical error
	MsgInf &inf; //modbus exception code is in inf.err
	std::mutex mutex;
	std::condition_variable cond;
};
typedef std::shared_ptr<Request> RequestPtr;

const uint32_t kDefRetries = 3;
const long kDefRdTimeout = 500; //ms
const long kPerReadTimeout = 10; //ms

} //namespace {

struct Master::Impl : public Task
{
public:
	typedef Net<IProtocol> INet;
	typedef Rtu<IProtocol> IRtu;
	typedef Ascii<IProtocol> IAscii;

	Impl(eThreadMode thrm)
		: retries_(kDefRetries)
		, rdto_(kDefRdTimeout)
		, thrm_(thrm)
	{
		if (thrm_ == TASK)
			this->Start();
	}

	~Impl()
	{
		if (thrm_ == TASK) {
			this->Stop();
			this->Wait();
		}
	}

	int Read(MsgInf &inf, uint8_t *buf, size_t bufsiz)
	{
		if (buf == nullptr) {
			//We don't need any response datas here.
			//But store will be updated after here.
			return this->SendRequest(inf);
		}

		//Because inf.databuf is a valid pointer of inf.pbuf
		//So we must guareentee inf.pbuf is valid after SendReuqest
		uint8_t msgbuf[kMaxMsgLen];
		inf.pbuf = msgbuf;
		inf.bufsiz = sizeof(msgbuf);

		int ret = this->SendRequest(inf);
		if (ret == 0 && inf.datalen != 0) { //received data
			if (bufsiz >= inf.datalen) {
				YMB_ASSERT(inf.databuf != nullptr);
				memcpy(buf, inf.databuf, inf.datalen);
				ret = inf.datalen;
			}
			else { //buffer size is too small to fit data
				YMB_DEBUG("buffer size is too small!\n");
				ret = -ENOMEM;
			}
		}

		return ret;
	}

	int Write(MsgInf &inf)
	{
		uint8_t sid = inf.id;
		uint8_t fun = inf.fun;
		uint16_t reg = inf.wreg;

		if (int ret = this->SendRequest(inf)) {
			YMB_DEBUG("eXecute Write failed!\n");
			return ret;
		}

		if (inf.err != 0) {
			YMB_DEBUG("eXecute Write exception! code = %u\n", inf.err);
			return -EFAULT;
		}

		if (inf.id != sid || inf.fun != fun || inf.wreg != reg) {
			YMB_DEBUG("eXecute Write response error id/fun/reg"
                "send: id = %02x, fun = %02x, reg = %02x \n"
                "recv: id = %02x, fun = %02x, reg = %02x \n",
                sid, fun, reg, inf.id, inf.fun, inf.wreg);
			return -EBADF;
		}

		return EOK;
	}

	int SendRequest(MsgInf &inf)
	{
		uint32_t retry = 0;
		error = -EFAULT;

		do {
			if (thrm_ == TASK) {
				if (auto req = std::make_shared<Request>(inf)) {
					//发送请求
					std::unique_lock<std::mutex> lockq(mutex_);
					requests_.push(req);
					cond_.notify_all();
					lockq.unlock();

					//等待执行
					std::unique_lock<std::mutex> lockr(req->mutex);
					req->cond.wait(lockr);

					//错误信息存放在线程局部变量
					error = req->err;
				}
			}
			else { //POLL, execute poll diretctly
				error = ExecPoll(inf);
			}
		} while (error != 0 && ++retry < retries_);

		return error;
	}

	void PostQuery(const MsgInf &inf)
	{
		if (thrm_ == TASK) {
			//提交查询，不需等待返回
			std::unique_lock<std::mutex> lock(mutex_);
			querys_.push(inf);
			cond_.notify_all();
		}
		else {
			//POLL mode, also need waiting for result, but only execute once
			error = ExecPoll(const_cast<MsgInf&>(inf));
		}
	}

	const long perto_ = kPerReadTimeout; //ms 每次接收超时
	uint32_t retries_;
	long rdto_; //read timeout
	eThreadMode thrm_;
	uint8_t msgbuf_[kMaxMsgLen];

	std::shared_ptr<IStore> store_;
	std::unique_ptr<IProtocol> prot_;
	std::unique_ptr<IConnect> conn_;
	static thread_local int error; //TMaster api operate error

protected:
	void Run(void) override; //thread func
	std::string Name(void) override { return "Master Task"; }

private:
	int ExecPoll(MsgInf &inf);
	int SendRecv(MsgInf &inf);

	std::queue<RequestPtr> requests_;
	std::queue<MsgInf> querys_;

	std::mutex mutex_;
	std::condition_variable cond_;
};

thread_local int Master::Impl::error = 0;

int Master::Impl::ExecPoll(MsgInf &inf)
{
	bool bInnerBuf;

	if (inf.pbuf == nullptr) {
		inf.pbuf = msgbuf_;
		inf.bufsiz = sizeof(msgbuf_);
		bInnerBuf = true;
	}
	else {
		YMB_ASSERT(inf.bufsiz >= sizeof(msgbuf_));
		bInnerBuf = false;
	}

	int ret = SendRecv(inf);

	if (ret == EOK && inf.datalen != 0 && store_) {
		YMB_ASSERT(inf.databuf != nullptr);
		store_->Set(inf.id, inf.rreg, inf.databuf, inf.rnum);
	}

	if (bInnerBuf) {
		inf.pbuf = nullptr;
		inf.bufsiz = 0;
		inf.databuf = nullptr;
		inf.datalen = 0;
	}

	return ret;
}

int Master::Impl::SendRecv(MsgInf &inf)
{
	if (!conn_->Validate())
		return -ENOLINK;

	YMB_ASSERT(inf.pbuf != nullptr);
	size_t msglen = prot_->MakeMasterMsg(inf.pbuf, inf.bufsiz, inf);

	conn_->Purge(); //清空buffer

	YMB_HEXDUMP0(inf.pbuf, msglen, "send: ");
	if (!conn_->Send(inf.pbuf, msglen))
		return -ENETRESET;

	msglen = 0;
	for (long to = 0; to < rdto_; to += perto_) { //timeout
		int ret = conn_->Recv(inf.pbuf + msglen, inf.bufsiz - msglen);
		if (ret > 0) { //received some data
		    YMB_HEXDUMP0(inf.pbuf + msglen, ret, "recv: ");
			msglen += ret;
			ret = prot_->VerifySlaveMsg(inf.pbuf, msglen);
			if (ret == EOK) //OK, msg arrived
				return prot_->ParseSlaveMsg(inf.pbuf, msglen, inf);
			if (ret < 0) //msg error
				return -EBADMSG;
		}
		if (ret < 0) //connect error
			return -ENETRESET;
	}

	return -EBUSY;
}

void Master::Impl::Run(void)
{
	while (this->IsRunning()) {
		//等待操作通知
		std::unique_lock<std::mutex> lock(mutex_);
		cond_.wait_for(lock, std::chrono::seconds(1));

		while (!requests_.empty() || !querys_.empty()) {
			if (!requests_.empty()) { //始终优先执行请求命令
				auto req = requests_.front();
				lock.unlock();

				req->err = ExecPoll(req->inf);
				req->cond.notify_all();

				lock.lock();
				requests_.pop();
			}
			else if (!querys_.empty()) {
				auto &inf = querys_.front();
				lock.unlock();

				ExecPoll(inf);

				lock.lock();
				querys_.pop();
			}
			else {
				; //assert(false);
			}
		}
	}

	//完成所有等待的请求
	std::unique_lock<std::mutex> lock(mutex_);
	while (!requests_.empty()) {
		auto req = requests_.front();
		lock.unlock();

		req->err = ExecPoll(req->inf);
		req->cond.notify_all();

		lock.lock();
		requests_.pop();
	}
}

//Master api implementation
Master::Master(const std::string &ip, uint16_t port,
	eProtocol type, eThreadMode thrm)
	: impl_(std::make_shared<Impl>(thrm))
{
	switch (type) {
	case TCP:
		impl_->prot_ = std::make_unique<Impl::INet>();
		impl_->conn_ = std::make_unique<TcpConnect>(ip, port);
		break;
	case TCPRTU:
		impl_->prot_ = std::make_unique<Impl::IRtu>();
		impl_->conn_ = std::make_unique<TcpConnect>(ip, port);
		break;
	case TCPASCII:
		impl_->prot_ = std::make_unique<Impl::IAscii>();
		impl_->conn_ = std::make_unique<TcpConnect>(ip, port);
		break;
	case UDP:
		impl_->prot_ = std::make_unique<Impl::INet>();
		impl_->conn_ = std::make_unique<UdpConnect>(ip, port);
		break;
	case UDPRTU:
		impl_->prot_ = std::make_unique<Impl::IRtu>();
		impl_->conn_ = std::make_unique<UdpConnect>(ip, port);
		break;
	case UDPASCII:
		impl_->prot_ = std::make_unique<Impl::IAscii>();
		impl_->conn_ = std::make_unique<UdpConnect>(ip, port);
		break;
	default:
		YMB_ASSERT(false);
		break;
	}

	impl_->conn_->SetTimeout(impl_->perto_);
	impl_->conn_->Validate();
}

Master::Master(const std::string &com, uint32_t baudrate,
	char parity, uint8_t stopbits, eProtocol type, eThreadMode thrm)
	: impl_(std::make_shared<Impl>(thrm))
{
	switch (type) {
	case RTU:
		impl_->prot_ = std::make_unique<Impl::IRtu>();
		impl_->conn_ = std::make_unique<SerConnect>(com,
			baudrate, 8, parity, stopbits);
		break;
	case ASCII:
		impl_->prot_ = std::make_unique<Impl::IAscii>();
		impl_->conn_ = std::make_unique<SerConnect>(com,
			baudrate, 7, parity, stopbits);
		break;
	default:
		YMB_ASSERT(false);
		break;
	}

	impl_->conn_->SetTimeout(impl_->perto_);
	impl_->conn_->Validate();
}

Master::~Master()
{

}

void Master::SetStore(std::shared_ptr<IStore> store)
{
	impl_->store_ = store;
}

std::shared_ptr<IStore> Master::GetStore(void) const
{
	return impl_->store_;
}

//reties: retry count
void Master::SetRetries(uint32_t retries)
{
	impl_->retries_ = retries;
}

uint32_t Master::GetRetries(void) const
{
	return impl_->retries_;
}

//rto: ms
void Master::SetReadTimeout(long rdto)
{
	impl_->rdto_ = rdto;
}

long Master::GetReadTimeout(void) const
{
	return impl_->rdto_;
}

//错误信息
int Master::GetLastError(void) const
{
	return impl_->error;
}

std::string Master::GetErrorString(int err) const
{
	return std::to_string(err);
}

bool Master::CheckConnect(void)
{
	YMB_ASSERT(impl_->conn_);
	return impl_->conn_->Validate();
}

//异步抓取操作，不需返回读取的数据，也不需等待执行完成
//返回的数据通过过SetStore的对象处理
void Master::PullCoils(uint8_t sid, uint16_t reg, uint16_t num)
{
	impl_->PostQuery({ sid, kFunReadCoils, reg, num });
}

void Master::PullDiscreteInputs(uint8_t sid, uint16_t reg, uint16_t num)
{
	impl_->PostQuery({ sid, kFunReadDiscreteInputs, reg, num });
}

void Master::PullHoldingRegisters(uint8_t sid, uint16_t reg, uint16_t num)
{
	impl_->PostQuery({ sid, kFunReadHoldingRegisters, reg, num });
}

void Master::PullInputRegisters(uint8_t sid, uint16_t reg, uint16_t num)
{
	impl_->PostQuery({ sid, kFunReadInputRegisters, reg, num });
}

//IPlayer================================================================
//return: >= 0, bytes of data to return;
//return: < 0, errorcode of exception
//buf: data value, net order
int Master::ReadCoils(uint8_t sid,
	uint16_t reg, uint16_t num, uint8_t *buf, size_t bufsiz)
{
	MsgInf inf = { sid, kFunReadCoils, reg, num };

	return impl_->Read(inf, buf, bufsiz);
}

int Master::ReadDiscreteInputs(uint8_t sid,
	uint16_t reg, uint16_t num, uint8_t *buf, size_t bufsiz)
{
	MsgInf inf = { sid, kFunReadDiscreteInputs, reg, num };

	return impl_->Read(inf, buf, bufsiz);
}

int Master::ReadInputRegisters(uint8_t sid,
	uint16_t reg, uint16_t num, uint8_t *buf, size_t bufsiz)
{
	MsgInf inf = { sid, kFunReadInputRegisters, reg, num };

	return impl_->Read(inf, buf, bufsiz);
}

int Master::ReadHoldingRegisters(uint8_t sid,
	uint16_t reg, uint16_t num, uint8_t *buf, size_t bufsiz)
{
	MsgInf inf = { sid, kFunReadHoldingRegisters, reg, num };

	return impl_->Read(inf, buf, bufsiz);
}

//return: = 0, OK;
//return: < 0, errorcode of exception
//values: data value, net order
int Master::WriteSingleCoil(uint8_t sid, uint16_t reg, bool onoff)
{
	uint8_t databuf[] = {
		static_cast<uint8_t>(onoff ? 0xff : 0x00),
		0x00
	};
	MsgInf inf = { sid, kFunWriteSingleCoil, 0, 0, reg, 1, databuf, 2 };

	return impl_->Write(inf);
}

int Master::WriteCoils(uint8_t sid,
	uint16_t reg, uint16_t num, const uint8_t *bits, uint8_t wbytes)
{
	MsgInf inf = { sid, kFunWriteMultiCoils, 0, 0, reg, num };

	inf.databuf = const_cast<uint8_t*>(bits);
	inf.datalen = wbytes;

	return impl_->Write(inf);
}

int Master::WriteSingleRegister(uint8_t sid, uint16_t reg, uint16_t value)
{
	uint8_t databuf[] = {
		static_cast<uint8_t>(value >> 8),
		static_cast<uint8_t>(value & 0xff)
	};
	MsgInf inf = { sid, kFunWriteSingleRegister, 0, 0, reg, 1, databuf, 2 };

	return impl_->Write(inf);
}

int Master::WriteRegisters(uint8_t sid,
	uint16_t reg, uint16_t num, const uint8_t *values, uint8_t wbytes)
{
	MsgInf inf = { sid, kFunWriteMultiRegisters, 0, 0, reg, num };

	inf.databuf = const_cast<uint8_t*>(values);
	inf.datalen = wbytes;

	return impl_->Write(inf);
}

int Master::MaskWriteRegisters(uint8_t sid,
	uint16_t reg, uint16_t andmask, uint16_t ormask)
{
	uint8_t databuf[] = {
		static_cast<uint8_t>(andmask >> 8),
		static_cast<uint8_t>(andmask & 0xff),
		static_cast<uint8_t>(ormask >> 8),
		static_cast<uint8_t>(ormask & 0xff)
	};
	MsgInf inf = { sid, kFunMaskWriteRegister, 0, 0, reg, 1, databuf, 4 };

	return impl_->Write(inf);
}

//return: >= 0, bytes of data to return
//return: < 0,  errorcode of exception
//values/buf: data value, net order
int Master::WriteReadRegisters(uint8_t sid,
	uint16_t wreg, uint16_t wnum, const uint8_t *values, uint8_t wbytes,
	uint16_t rreg, uint16_t rnum, uint8_t *buf, size_t bufsiz)
{
	MsgInf inf = { sid, kFunWriteAndReadRegisters, rreg, rnum, wreg, wnum };

	inf.databuf = const_cast<uint8_t*>(values);
	inf.datalen = wbytes;

	return impl_->Read(inf, buf, bufsiz);
}

//return: >= 0, OK
//return: < 0,  errorcode of exception
int Master::ReportSlaveId(uint8_t /* maxsid */, uint8_t * /* buf */, size_t /* bufsiz*/)
{
	YMB_DEBUG("%s%s not implementation.\n", __FILE__, __func__);

	return -1;
}

} //namespace ymodbus
