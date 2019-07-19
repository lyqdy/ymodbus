/**
* ymodbus
* Copyright © 2019-2019 liuyongqing<lyqdy1@163.com>
* v1.0.1 2019.05.04
*/
#ifndef __YMODBUS_YMASTER_H__
#define __YMODBUS_YMASTER_H__

#include "ymod/ymbdefs.h"
#include "ymod/ymbstore.h"
#include "ymod/ymbplayer.h"
#include "ymod/ymbtask.h"

#include "ymod/ymbnet.h"
#include "ymod/ymbrtu.h"
#include "ymod/ymbascii.h"
#include "ymod/ymbutils.h"

#include "ymod/master/ytcpconnect.h"
#include "ymod/master/yserconnect.h"
#include "ymod/master/yudpconnect.h"

#include "ymblog.h"
#include "ymbopts.h"

#include <string>
#include <vector>
#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <map>

namespace YModbus {

struct MNullInterface {};

typedef Net<MNullInterface> MNet;
typedef Rtu<MNullInterface> MRtu;
typedef Ascii<MNullInterface> MAscii;

//TProtocol: {MNet, MRtu, MAscii}
//TConnect: {TcpConnect, SerConnect, UdpConnect}
//TBase: {MNullInterface, IPlayer}
template<
	typename TProtocol, 
	typename TConnect, 
	typename TBase = MNullInterface>
class TMaster 
	: public Task //for thread mode
	, public TBase
{
public:
	TMaster() = delete;
	TMaster(const TMaster&) = delete;
	TMaster &operator = (const TMaster &t) = delete;
	TMaster(TMaster &&t) = delete;
	TMaster &operator = (TMaster &&t) = delete;

	//TMaster api implementation
	TMaster(const std::string &ip, uint16_t port, eThreadMode thrm)
		: thrm_(thrm)
		, bor_(BOR_1234)
		, conn_(ip, port)
	{
		this->conn_.SetTimeout(this->perto_);
		this->conn_.Validate();

		if (this->thrm_ == TASK)
			this->Start();
	}

	TMaster(const std::string &com, uint32_t baudrate,
		char parity, uint8_t stopbits, eThreadMode thrm)
		: thrm_(thrm)
		, bor_(BOR_1234)
		, conn_(com, baudrate, 8, parity, stopbits)
	{
		this->conn_.SetTimeout(this->perto_);
		this->conn_.Validate();

		if (thrm_ == TASK)
			this->Start();
	}

	~TMaster()
	{
		WaitAsynReader();

		if (thrm_ == TASK) {
			this->Stop();
			this->Wait();
		}
	}

	void WaitAsynReader(void)
	{
		//wait for all of reader exit
		std::unique_lock<std::mutex> lock(asynMutex_);
		while (!asynReader_.empty()) {
			auto it = asynReader_.begin();
			auto id = it->first;
			auto reader = std::move(it->second);
			lock.unlock();
			if (reader.joinable())
				reader.join();
			lock.lock();
			it = asynReader_.find(id);
			if (it != asynReader_.end()) 
				asynReader_.erase(id);
		}
	}

	void SetByteOrder(eByteOrder bor) {	bor_ = bor; }
	eByteOrder GetByteOrder(void) const { return bor_; }

	void SetStore(std::shared_ptr<IStore> store) { store_ = store; }
	std::shared_ptr<IStore> GetStore(void) const { return store_; }

	//reties: retry count
	void SetRetries(uint32_t retries) { retries_ = retries; }
	uint32_t GetRetries(void) const { return retries_; }

	//rdto: ms
	void SetReadTimeout(long rdto) { rdto_ = rdto; }
	long GetReadTimeout(void) const { return rdto_; }

	bool CheckConnect(void)
	{
		YMB_ASSERT(this->conn_);
		return this->conn_.Validate();
	}

	//异步抓取操作，不需返回读取的数据，也不需等待执行完成
	//返回的数据通过过SetStore的对象处理
	void PullCoils(uint8_t sid, uint16_t reg, uint16_t num)
	{
		this->PostQuery({ sid, kFunReadCoils, reg, num });
	}

	void PullDiscreteInputs(uint8_t sid, uint16_t reg, uint16_t num)
	{
		this->PostQuery({ sid, kFunReadDiscreteInputs, reg, num });
	}

	void PullHoldingRegisters(uint8_t sid, uint16_t reg, uint16_t num)
	{
		this->PostQuery({ sid, kFunReadHoldingRegisters, reg, num });
	}

	void PullInputRegisters(uint8_t sid, uint16_t reg, uint16_t num)
	{
		this->PostQuery({ sid, kFunReadInputRegisters, reg, num });
	}

	//IPlayer================================================================
	//return: >= 0, bytes of data to return;
	//return: < 0, errorcode of exception
	//buf: data value, net order
	int ReadCoils(uint8_t sid,
		uint16_t reg, uint16_t num, uint8_t *buf, size_t bufsiz)
	{
		MsgInf inf = { sid, kFunReadCoils, reg, num };

		return this->Read(inf, buf, bufsiz);
	}

	int ReadDiscreteInputs(uint8_t sid,
		uint16_t reg, uint16_t num, uint8_t *buf, size_t bufsiz)
	{
		MsgInf inf = { sid, kFunReadDiscreteInputs, reg, num };

		return this->Read(inf, buf, bufsiz);
	}

	int ReadInputRegisters(uint8_t sid,
		uint16_t reg, uint16_t num, uint8_t *buf, size_t bufsiz)
	{
		MsgInf inf = { sid, kFunReadInputRegisters, reg, num };

		return this->Read(inf, buf, bufsiz);
	}

	int ReadHoldingRegisters(uint8_t sid,
		uint16_t reg, uint16_t num, uint8_t *buf, size_t bufsiz)
	{
		MsgInf inf = { sid, kFunReadHoldingRegisters, reg, num };

		return this->Read(inf, buf, bufsiz);
	}

	//return: = 0, OK; 
	//return: < 0, errorcode of exception
	//values: data value, net order
	int WriteSingleCoil(uint8_t sid, uint16_t reg, bool onoff)
	{
		uint8_t databuf[] = {
			static_cast<uint8_t>(onoff ? 0xff : 0x00),
			0x00
		};
		MsgInf inf = { sid, kFunWriteSingleCoil, 0, 0, reg, 1, databuf, 2 };

		return this->Write(inf);
	}

	int WriteCoils(uint8_t sid,
		uint16_t reg, uint16_t num, const uint8_t *bits, uint8_t wbytes)
	{
		MsgInf inf = { sid, kFunWriteMultiCoils, 0, 0, reg, num };

		inf.databuf = const_cast<uint8_t*>(bits);
		inf.datalen = wbytes;

		return this->Write(inf);
	}

	int WriteSingleRegister(uint8_t sid, uint16_t reg, uint16_t value)
	{
		uint8_t databuf[] = {
			static_cast<uint8_t>(value >> 8),
			static_cast<uint8_t>(value & 0xff)
		};
		MsgInf inf = { sid, kFunWriteSingleRegister, 0, 0, reg, 1, databuf, 2 };

		return this->Write(inf);
	}

	int WriteRegisters(uint8_t sid,
		uint16_t reg, uint16_t num, const uint8_t *values, uint8_t wbytes)
	{
		MsgInf inf = { sid, kFunWriteMultiRegisters, 0, 0, reg, num };

		inf.databuf = const_cast<uint8_t*>(values);
		inf.datalen = wbytes;

		return this->Write(inf);
	}

	int MaskWriteRegisters(uint8_t sid,
		uint16_t reg, uint16_t andmask, uint16_t ormask)
	{
		uint8_t databuf[] = {
			static_cast<uint8_t>(andmask >> 8),
			static_cast<uint8_t>(andmask & 0xff),
			static_cast<uint8_t>(ormask >> 8),
			static_cast<uint8_t>(ormask & 0xff)
		};
		MsgInf inf = { sid, kFunMaskWriteRegister, 0, 0, reg, 1, databuf, 4 };

		return this->Write(inf);
	}

	//return: >= 0, bytes of data to return
	//return: < 0,  errorcode of exception
	//values/buf: data value, net order
	int WriteReadRegisters(uint8_t sid,
		uint16_t wreg, uint16_t wnum, const uint8_t *values, uint8_t wbytes,
		uint16_t rreg, uint16_t rnum, uint8_t *buf, size_t bufsiz)
	{
		MsgInf inf = { sid, kFunWriteAndReadRegisters, rreg, rnum, wreg, wnum };

		inf.databuf = const_cast<uint8_t*>(values);
		inf.datalen = wbytes;

		return this->Read(inf, buf, bufsiz);
	}

	//return: >= 0, OK
	//return: < 0,  errorcode of exception
	int ReportSlaveId(uint8_t maxsid, uint8_t *buf, size_t bufsiz)
	{
		YMB_DEBUG("%s%s not implementation.\n", __FILE__, __func__);

		return -1;
	}

	template<typename T>
	bool ReadValue(uint8_t sid, uint16_t startreg, T &val)
	{
		T value; 
		
		//holding/input? TODO
		int ret = ReadHoldingRegisters(sid, startreg, sizeof(value)/2,
			reinterpret_cast<uint8_t*>(&value), sizeof(value));

		YNetToHost(value, bor_);
		val = value;

		return ret == sizeof(T);
	}

	template<typename T>
	bool WriteValue(uint8_t sid, uint16_t startreg, T &val)
	{
		T value = val;
		YHostToNet(value, bor_);
		
		int ret = WriteReadRegisters(sid, startreg, sizeof(value) / 2,
			reinterpret_cast<uint8_t*>(&value), sizeof(value));

		return ret == EOK;
	}

	template<typename Tw, typename Tr>
	bool WriteReadValue(uint8_t sid, 
		uint16_t wreg, Tw &wval, uint16_t rreg, Tr &rval)
	{
		Tw wvalue = wval;
		Tr rvalue;

		YHostToNet(wvalue, bor_);

		int ret = ReadHoldingRegisters(sid,
			wreg, sizeof(wvalue) / 2,
			reinterpret_cast<uint8_t*>(&wvalue), sizeof(wvalue),
			rreg, sizeof(rvalue) / 2,
			reinterpret_cast<uint8_t*>(&rvalue), sizeof(rvalue));

		YNetToHost(rvalue, bor_);
		rval = rvalue;

		return ret == sizeof(rvalue);
	}

	template<typename T, typename F>
	void AsyncRead(uint8_t sid, uint16_t startreg, F f)
	{
		std::unique_lock<std::mutex> lock(asynMutex_);

		for (auto id : exitReader_) {	//erase exit reader thread
			auto it = asynReader_.find(id);
			if (it != asynReader_.end()) {
				auto reader = std::move(it->second);
				if (reader.joinable())
					reader.join();
				asynReader_.erase(it);
			}
		}
		exitReader_.clear();

		asynReader_[asynId_] = std::thread(
			[this](uint8_t s, uint16_t r, F f, uint32_t id) {
			T val;
			if (this->ReadValue(s, r, val)) 
				f(s, r, val);
			std::unique_lock<std::mutex> lock(this->asynMutex_);
			this->exitReader_.push_back(id); //mark exit reader
		}, sid, startreg, f, asynId_);

		asynId_++;
	}

protected:
	void Run(void) override; //thread func
	std::string Name(void) override { return "TMaster Task"; }

private:
	int ExecPoll(MsgInf &inf);
	int SendRecv(MsgInf &inf);

	int Read(MsgInf &inf, uint8_t *buf, size_t bufsiz);
	int Write(MsgInf &inf);

	int SendRequest(MsgInf &inf);
	void PostQuery(const MsgInf &inf);

	const uint32_t kDefRetries = 3;
	const long kDefRdTimeout = 500; //ms
	const long kPerReadTimeout = 10; //ms

	uint32_t retries_ = kDefRetries;
	long rdto_ = kDefRdTimeout; //read timeout
	const long perto_ = kPerReadTimeout; //ms 每次接收超时

	eThreadMode thrm_;
	eByteOrder bor_;
	uint8_t msgbuf_[kMaxMsgLen];

	std::shared_ptr<IStore> store_;
	TProtocol prot_;
	TConnect conn_;

	struct Request
	{
		Request(MsgInf &i) : err(0), inf(i) {}
		int err;	//error, api or logical error
		MsgInf &inf; //modbus exception code is in inf.err
		std::mutex mutex;
		std::condition_variable cond;
	};
	typedef std::shared_ptr<Request> RequestPtr;

	std::queue<RequestPtr> requests_;
	std::queue<MsgInf> querys_;

	std::mutex mutex_;
	std::condition_variable cond_;

	uint32_t asynId_ = 0;
	std::mutex asynMutex_;
	std::map<uint32_t, std::thread> asynReader_;
	std::vector<uint32_t> exitReader_;

	static thread_local int error;
};

template<typename TProtocol, typename TConnect, typename TBase>
thread_local int TMaster<TProtocol, TConnect, TBase>::error = 0;

template<typename TProtocol, typename TConnect, typename TBase>
int TMaster<TProtocol, TConnect, TBase>::Read(MsgInf &inf,
	uint8_t *buf, size_t bufsiz)
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

template<typename TProtocol, typename TConnect, typename TBase>
int  TMaster<TProtocol, TConnect, TBase>::Write(MsgInf &inf)
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
		YMB_DEBUG("eXecute Write response error id/fun/reg\n");
		return -EFAULT;
	}

	return EOK;
}

template<typename TProtocol, typename TConnect, typename TBase>
int  TMaster<TProtocol, TConnect, TBase>::SendRequest(MsgInf &inf)
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

template<typename TProtocol, typename TConnect, typename TBase>
void  TMaster<TProtocol, TConnect, TBase>::PostQuery(const MsgInf &inf)
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

template<typename TProtocol, typename TConnect, typename TBase>
int TMaster<TProtocol, TConnect, TBase>::ExecPoll(MsgInf &inf)
{
	bool bInnerBuf = false;

	if (inf.pbuf == nullptr) {
		inf.pbuf = msgbuf_;
		inf.bufsiz = sizeof(msgbuf_);
		bInnerBuf = true;
	}

	YMB_ASSERT(inf.bufsiz >= sizeof(msgbuf_));

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

template<typename TProtocol, typename TConnect, typename TBase>
int TMaster<TProtocol, TConnect, TBase>::SendRecv(MsgInf &inf)
{
	if (!conn_.Validate())
		return -ENOLINK;

	YMB_ASSERT(inf.pbuf != nullptr);
	size_t msglen = prot_.MakeMasterMsg(inf.pbuf, inf.bufsiz, inf);

	conn_.Purge(); //清空buffer
	if (!conn_.Send(inf.pbuf, msglen))
		return -ENETRESET;

	msglen = 0;
	for (long to = 0; to < rdto_; to += perto_) { //timeout
		int ret = conn_.Recv(inf.pbuf + msglen, inf.bufsiz - msglen);
		if (ret > 0) { //received some data
			msglen += ret;
			ret = prot_.VerifySlaveMsg(inf.pbuf, msglen);
			if (ret == EOK) //OK, msg arrived
				return prot_.ParseSlaveMsg(inf.pbuf, msglen, inf);
			if (ret < 0) //msg error
				return -EBADMSG;
		}
		if (ret < 0) //connect error
			return -ENETRESET;
	}

	return -EBUSY;
}

template<typename TProtocol, typename TConnect, typename TBase>
void TMaster<TProtocol, TConnect, TBase>::Run(void)
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

//The most useful master predefines
typedef TMaster<MNet, TcpConnect> TcpMaster;
typedef TMaster<MRtu, SerConnect> RtuMaster;
typedef TMaster<MAscii, SerConnect> AsciiMaster;

} //namespace YModbus

#endif //__YMODBUS_YMASTER_H__
