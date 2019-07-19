/**
* ymodbus
* Copyright © 2019-2019 liuyongqing<lyqdy1@163.com>
* v1.0.1 2019.05.04
*/
#ifndef __YMODBUS_YSLAVE_H__
#define __YMODBUS_YSLAVE_H__

#include "ymod/slave/ytcplistener.h"
#include "ymod/slave/yudplistener.h"
#include "ymod/slave/yserlistener.h"

#include "ymod/ymbplayer.h"
#include "ymod/ymbnet.h"
#include "ymod/ymbrtu.h"
#include "ymod/ymbascii.h"
#include "ymod/ymbtask.h"
#include "ymod/ymbdefs.h"

#include "ymblog.h"
#include "ymbopts.h"

#include <string>
#include <memory>

namespace YModbus {
	
struct SNullInterface {};

typedef Net<SNullInterface> SNet;
typedef Rtu<SNullInterface> SRtu;
typedef Ascii<SNullInterface> SAscii;

//TProtocol: {SNet, SRtu, SAscii}
//TListener: {TcpListener, UdpListener,SerListener}
template<
	typename TProtocol,
	typename TListener,
	typename TPlayer = IPlayer
>
class TSlave : public Task 
{
public:
	TSlave() = delete;
	TSlave(const TSlave&) = delete;
	TSlave(TSlave&&) = delete;
	TSlave& operator=(TSlave&&) = delete;

	TSlave(const std::string& port,
		uint32_t baudrate,
		uint8_t databits,
		char parity,
		uint8_t stopbits,
		eThreadMode thrm)
		: thrm_(thrm)
		, listener_(port, baudrate, databits, parity, stopbits)
	{
		this->listener_.SetTimeout(kDefListenTimeout);
		YMB_DEBUG("Create modbus TSlave server: %s: %s, mode: %s\n",
			this->listener_.GetName().c_str(), TProtocol::protname,
			thrm == POLL ? "POLL" : "TASK");
	}

	TSlave(uint16_t port, eThreadMode thrm)
		: thrm_(thrm)
		, listener_(port)
	{
		this->listener_.SetTimeout(kDefListenTimeout);
		YMB_DEBUG("Create modbus TSlave server: %s: %s, mode: %s\n",
			this->listener_.GetName().c_str(), TProtocol::protname,
			thrm == POLL ? "POLL" : "TASK");
	}

	~TSlave()
	{

	}

	//By default, any id will receipt(id=0)
	void SetSlaveId(uint8_t id)
	{
		this->id_ = id;
	}

	uint8_t GetSlaveId(void) const
	{
		return this->id_;
	}

	void SetPlayer(std::shared_ptr<TPlayer> player)
	{
		this->player_ = player;
	}

	std::shared_ptr<TPlayer> GetPlayer(void) const
	{
		return this->player_;
	}

	bool Startup(void)
	{
		if (!this->listener_.Listen()) {
			LOG(ERROR) << "TSlave Listen failed.";
			return false;
		}

		if (this->thrm_ == TASK) {
			this->Start();
		}

		return true;
	}

	void Shutdown(void)
	{
		if (this->thrm_ == TASK) {
			this->Stop();
			this->Wait();
		}
	}

	//for nthr = 0 to loop
	//to: timeout: ms
	//return: < 0: errorcode, = 0: no error
	int Run(long to)
	{
		YMB_ASSERT(this->thrm_ == POLL);
		
		listener_.SetTimeout(to);
		Accept();

		return err_;
	}

	int GetLastError(void) const
	{
		return this->err_;
	}

protected:
	virtual void Run(void) override
	{
		while (IsRunning()) {
			Accept();
		}
	}

	virtual std::string Name(void) override { return "TSlave Task"; }

private:
	void Accept(void);
	int Request(MsgInf &inf, uint8_t *rspbuf, size_t bufsiz);
	
	const long kDefListenTimeout = 1000; //ms
	
	eThreadMode thrm_;

	int err_ = 0;
	uint8_t id_ = kAnySlaveId; //TSlave id

	TProtocol prot_;
	std::shared_ptr<TPlayer> player_;

	TListener listener_;
	std::vector<SessionPtr> ses_;

	uint8_t rspbuf_[kMaxMsgLen];
	size_t bufsiz_ = kMaxMsgLen;
};

template<typename TProtocol, typename TListener, typename TPlayer>
void TSlave<TProtocol, TListener, TPlayer>::Accept(void)
{
	MsgInf inf;
	uint8_t *recvmsg;

	err_ = listener_.Accept(ses_);
	for (auto &session : ses_) {
		size_t msglen = session->Peek(&recvmsg);
		int need = prot_.VerifyMasterMsg(recvmsg, msglen);
		if (need < 0) { //bad msg
			YMB_HEXDUMP(recvmsg, msglen,
				"Bad master message! len = %u:", msglen);
			session->Purge(); //error, clear port data
			continue;
		}

		//msg has arrived
		if (need == 0 && prot_.ParseMasterMsg(recvmsg, msglen, inf) == EOK) {
			if (inf.id == id_ || id_ == kAnySlaveId) { //token or careless id
				size_t roff = prot_.GetSlaveDataOffset(inf.fun);
				int rsp = Request(inf, rspbuf_ + roff, bufsiz_ - roff);
				if (rsp >= 0 && inf.id != kBroadcastId) {
					inf.databuf = nullptr; //The Datas have filled into rspbuf.
					msglen = prot_.MakeSlaveMsg(rspbuf_, bufsiz_, inf);
					session->Purge();	//clear port before response
					session->Write(rspbuf_, msglen);
				} //exec ok
			} //id tocken
		} //request
	} //for ses_
}

template<typename TProtocol, typename TListener, typename TPlayer>
int TSlave<TProtocol, TListener, TPlayer>::Request(MsgInf &inf,
	uint8_t *rdbuf, size_t rdbufsiz)
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

//The most useful slave predefines
typedef TSlave<SNet, TcpListener> TcpSlave;
typedef TSlave<SRtu, SerListener> RtuSlave;
typedef TSlave<SAscii, SerListener> AsciiSlave;

} //namespace YModbus

#endif // ! __YMODBUS_YSLAVE_H__

