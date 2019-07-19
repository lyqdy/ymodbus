/**
* ymodbus
* Copyright © 2019-2019 liuyongqing<lyqdy1@163.com>
* v1.0.1 2019.05.04
*/
#include "ymod/slave/ymbslave.h"
#include "ymod/slave/yserlistener.h"
#include "ymod/slave/ytcplistener.h"
#include "ymod/slave/yudplistener.h"

#include "ymod/ymbnet.h"
#include "ymod/ymbrtu.h"
#include "ymod/ymbascii.h"
#include "ymod/ymbtask.h"

#include "ymblog.h"
#include "ymbopts.h"

#include <vector>

namespace YModbus {
	
namespace {

const long kDefListenTimeout = 1000; //ms

} //namespace {

struct Slave::Impl : public Task
{
	typedef Net<IProtocol> Net;
	typedef Rtu<IProtocol> Rtu;
	typedef Ascii<IProtocol> Ascii;

	Impl(eThreadMode thrm) : thrm_(thrm) {}
	~Impl() {}

	int Run(long to)
	{
		listener_->SetTimeout(to);
		Accept();
		return err_;
	}

	int Request(MsgInf &inf, uint8_t *rspbuf, size_t bufsiz);

	eThreadMode thrm_;

	int err_ = 0;
	uint8_t id_ = kAnySlaveId; //slave id

	std::unique_ptr<IProtocol> prot_;
	std::shared_ptr<IPlayer> player_;

	std::unique_ptr<IListerner> listener_;
	std::vector<SessionPtr> ses_;

	uint8_t rspbuf_[kMaxMsgLen];
	size_t bufsiz_ = kMaxMsgLen;

protected:
	virtual void Run(void) override;
	virtual std::string Name(void) override { return "Slave Task"; }

private:
	void Accept(void);
};

void Slave::Impl::Run(void)
{
	while (IsRunning()) {
		Accept();
	}
}

void Slave::Impl::Accept(void)
{
	MsgInf inf;
	uint8_t *recvmsg;

	err_ = listener_->Accept(ses_);
	for (auto &session : ses_) {
		size_t msglen = session->Peek(&recvmsg);
		int need = prot_->VerifyMasterMsg(recvmsg, msglen);
		if (need < 0) { //bad msg
			YMB_HEXDUMP(recvmsg, msglen, 
				"Bad master message! len = %u:\n", msglen);
			session->Purge(); //error, clear port data
			continue; 
		}

		//msg has arrived
		if (need == 0 && prot_->ParseMasterMsg(recvmsg, msglen, inf) == EOK) {
			if (inf.id == id_ || id_ == kAnySlaveId) { //token or careless id
				size_t roff = prot_->GetSlaveDataOffset(inf.fun);
				int rsp = Request(inf, rspbuf_ + roff, bufsiz_ - roff);
				if (rsp >= 0 && inf.id != kBroadcastId) {
					inf.databuf = nullptr; //The Datas have filled into rspbuf.
					msglen = prot_->MakeSlaveMsg(rspbuf_, bufsiz_, inf);
					session->Purge();	//clear port before response
					session->Write(rspbuf_, msglen);
					YMB_HEXDUMP(rspbuf_, msglen,
						"Response message! len = %u: \n", msglen);
				}
			}
		}
	}
}

int Slave::Impl::Request(MsgInf &inf, uint8_t *rdbuf, size_t rdbufsiz)
{
	int rsp;

	YMB_ASSERT(player_ != nullptr);
	YMB_DEBUG0("ExecRequest id = %u, fun = %u\n", inf.id, inf.fun);

	switch (inf.fun) {
	case kFunReadCoils:
		rsp = player_->ReadCoils(inf.id,
			inf.rreg, inf.rnum, rdbuf, rdbufsiz);
		break;
	case kFunReadDiscreteInputs:
		rsp = player_->ReadDiscreteInputs(inf.id,
			inf.rreg, inf.rnum, rdbuf, rdbufsiz);
		break;
	case kFunReadHoldingRegisters:
		rsp = player_->ReadHoldingRegisters(inf.id,
			inf.rreg, inf.rnum, rdbuf, rdbufsiz);
		break;
	case kFunReadInputRegisters:
		rsp = player_->ReadInputRegisters(inf.id,
			inf.rreg, inf.rnum, rdbuf, rdbufsiz);
		break;
	case kFunWriteAndReadRegisters:
		rsp = player_->WriteReadRegisters(inf.id,
			inf.wreg, inf.wnum, inf.databuf, inf.datalen,
			inf.rreg, inf.rnum, rdbuf, rdbufsiz);
		break;
	case kFunWriteMultiCoils:
		rsp = player_->WriteCoils(inf.id,
			inf.wreg, inf.wnum, inf.databuf, inf.datalen);
		break;
	case kFunWriteMultiRegisters:
		rsp = player_->WriteRegisters(inf.id,
			inf.wreg, inf.wnum, inf.databuf, inf.datalen);
		break;
	case kFunWriteSingleCoil:
		YMB_ASSERT(inf.databuf != nullptr && inf.datalen == 2);
		rsp = player_->WriteSingleCoil(inf.id, inf.wreg,
			inf.databuf[0] == 0xff && inf.databuf[1] == 0x00);
		if (rsp == 0) {
			rsp = 2; //ack the same data of request
			memmove(rdbuf, inf.databuf, rsp);
		}
		break;
	case kFunWriteSingleRegister:
		YMB_ASSERT(inf.databuf != nullptr && inf.datalen == 2);
		rsp = player_->WriteSingleRegister(inf.id, inf.wreg,
			static_cast<uint16_t>((inf.databuf[0] << 8) | inf.databuf[1]));
		if (rsp == 0) {
			rsp = 2; //ack the same data of request
			memmove(rdbuf, inf.databuf, rsp);
		}
		break;
	case kFunMaskWriteRegister:
		rsp = player_->MaskWriteRegisters(inf.id, inf.wreg,
			static_cast<uint16_t>((inf.databuf[0] << 8) | inf.databuf[1]),
			static_cast<uint16_t>((inf.databuf[2] << 8) | inf.databuf[3]));
		if (rsp == 0) {
			rsp = 4; //ack the same data of request
			memmove(rdbuf, inf.databuf, rsp);
		}
		break;
	default:
		rsp = -EINVAL;
		break;
	}

	inf.err = rsp >= 0 ? 0 : static_cast<uint8_t>(-rsp);
	inf.datalen = rsp > 0 ? static_cast<uint8_t>(rsp) : 0;
	inf.databuf = inf.datalen != 0 ? rdbuf : nullptr;

	YMB_DEBUG0("Exec Result rsp = %u\n", rsp);

	return rsp;
}

//YMB_RTU/YMB_ASCII
Slave::Slave(const std::string& port,
	uint32_t baudrate,
	char parity,
	uint8_t stopbits,
	eProtocol prot,
	eThreadMode thrm)
	: impl_(std::make_shared<Impl>(thrm))
{
	switch (prot) {
	case RTU:
		impl_->listener_ = std::make_unique<SerListener>(port,
			baudrate, 8, parity, stopbits);
		impl_->prot_ = std::make_unique<Impl::Rtu>();
		break;
	case ASCII:
		impl_->listener_ = std::make_unique<SerListener>(port,
			baudrate, 7, parity, stopbits);
		impl_->prot_ = std::make_unique<Impl::Ascii>();
		break;
	default:
		YMB_ASSERT(false);
		break;
	}
	
	impl_->listener_->SetTimeout(kDefListenTimeout);

	YMB_DEBUG("Create modbus slave server: %s: %s, mode: %s\n",
		impl_->listener_->GetName().c_str(), PROTNAME(prot),
		thrm == POLL ? "POLL" : "TASK");
}

//YMB_TCP/YMB_UDP
//YMB_TCP_RTU/YMB_TCP_ASCII
//YMB_UDP_RTU/YMB_UDP_ASCII
Slave::Slave(uint16_t port, eProtocol prot, eThreadMode thrm)
	: impl_(std::make_shared<Impl>(thrm))
{
	switch (prot) {
	case TCP:
		impl_->listener_ = std::make_unique<TcpListener>(port);
		impl_->prot_ = std::make_unique<Impl::Net>();
		break;
	case TCPRTU:
		impl_->listener_ = std::make_unique<TcpListener>(port);
		impl_->prot_ = std::make_unique<Impl::Rtu>();
		break;
	case TCPASCII:
		impl_->listener_ = std::make_unique<TcpListener>(port);
		impl_->prot_ = std::make_unique<Impl::Ascii>();
		break;
	case UDP:
		impl_->listener_ = std::make_unique<UdpListener>(port);
		impl_->prot_ = std::make_unique<Impl::Net>();
		break;
	case UDPRTU:
		impl_->listener_ = std::make_unique<UdpListener>(port);
		impl_->prot_ = std::make_unique<Impl::Rtu>();
		break;
	case UDPASCII:
		impl_->listener_ = std::make_unique<UdpListener>(port);
		impl_->prot_ = std::make_unique<Impl::Ascii>();
		break;
	default:
		YMB_ASSERT(false);
		break;
	}

	impl_->listener_->SetTimeout(kDefListenTimeout);

	YMB_DEBUG("Create modbus slave server: %s: %s, mode: %s\n",
		impl_->listener_->GetName().c_str(), PROTNAME(prot),
		thrm == POLL ? "POLL" : "TASK");
}

Slave::~Slave()
{

}

//By default, any id will receipt(id=0)
void Slave::SetSlaveId(uint8_t id)
{
	impl_->id_ = id;
}

uint8_t Slave::GetSlaveId(void) const
{
	return impl_->id_;
}

void Slave::SetPlayer(std::shared_ptr<IPlayer> player)
{
	impl_->player_ = player;
}

std::shared_ptr<IPlayer> Slave::GetPlayer(void) const
{
	return impl_->player_;
}

bool Slave::Startup(void)
{
	if (!impl_->listener_->Listen()) {
		LOG(ERROR) << "Slave Listen failed.";
		return false;
	}
	
	if (impl_->thrm_ == TASK) {
		impl_->Start();
	}

	return true;
}

void Slave::Shutdown(void)
{
	if (impl_->thrm_ == TASK) {
		impl_->Stop();
		impl_->Wait();
	}
}

//for nthr = 0 to loop
//to: timeout: ms
//return: < 0: errorcode, = 0: no error
int Slave::Run(long to)
{
	YMB_ASSERT(impl_->thrm_ == POLL);

	return impl_->Run(to);
}

int Slave::GetLastError(void) const
{
	return impl_->err_;
}

} //namespace YModbus

