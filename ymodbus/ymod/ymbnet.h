/**
* ymodbus
* Copyright © 2019-2019 liuyongqing<lyqdy1@163.com>
* v1.0.1 2019.05.04
*/
#ifndef __YMODBUS_YMBNET_H__
#define __YMODBUS_YMBNET_H__

#include "ymod/ymbdefs.h"
#include "ymod/ymbprot.h"
#include "ymblog.h"

#include <memory>

namespace YModbus {

template<typename TBase>
class Net : public TBase
{
public:
	static const char *protname;

	int GetMasterDataOffset(uint8_t fun)
	{
		return Protocol::GetMasterDataOffset(fun) + kHdrSiz;
	}

	int GetSlaveDataOffset(uint8_t fun)
	{
		return Protocol::GetSlaveDataOffset(fun) + kHdrSiz;
	}

	size_t MakeMasterMsg(uint8_t *buf, size_t bufsiz, MsgInf &inf)
	{
		tid_++;

		//tid
		buf[0] = static_cast<uint8_t>(tid_ >> 8);
		buf[1] = static_cast<uint8_t>(tid_ & 0xff);

		//protocol type
		buf[2] = buf[3] = 0;

		size_t msglen = Protocol::MakeMasterMsg(buf + kHdrSiz, bufsiz - kHdrSiz, inf);
		YMB_ASSERT(bufsiz >= msglen + kHdrSiz);

		//Now, we got the msg's length
		buf[4] = static_cast<uint8_t>((msglen) >> 8);
		buf[5] = static_cast<uint8_t>((msglen) & 0xff);

		return msglen + kHdrSiz;
	}

	size_t MakeSlaveMsg(uint8_t *buf, size_t bufsiz, MsgInf &inf)
	{
		//tid
		buf[0] = static_cast<uint8_t>(tid_ >> 8);
		buf[1] = static_cast<uint8_t>(tid_ & 0xff);

		//protocol type
		buf[2] = buf[3] = 0;

		size_t msglen = Protocol::MakeSlaveMsg(buf + kHdrSiz, bufsiz - kHdrSiz, inf);
		YMB_ASSERT(bufsiz >= msglen + kHdrSiz);

		//fill length field
		buf[4] = static_cast<uint8_t>((msglen) >> 8);
		buf[5] = static_cast<uint8_t>((msglen) & 0xff);

		return msglen + kHdrSiz;
	}

	int VerifyMasterMsg(uint8_t *msg, size_t msglen)
	{
		if (msglen >= kHdrSiz)
			return Protocol::VerifyMasterMsg(msg + kHdrSiz, msglen - kHdrSiz);
			
		return static_cast<int>(kHdrSiz - msglen);
	}

	int VerifySlaveMsg(uint8_t *msg, size_t msglen)
	{
		if (msglen >= kHdrSiz)
			return Protocol::VerifySlaveMsg(msg + kHdrSiz, msglen - kHdrSiz);
		
		return static_cast<int>(kHdrSiz - msglen);
	}

	//Used by slave
	int ParseMasterMsg(uint8_t *msg, size_t msglen, MsgInf &inf)
	{
		//Buffer the tid for rsp msg
		tid_ = (msg[0] << 8) | msg[1];

		return Protocol::ParseMasterMsg(msg + kHdrSiz, msglen - kHdrSiz, inf);
	}

	//Used by master
	int ParseSlaveMsg(uint8_t *msg, size_t msglen, MsgInf &inf)
	{
		uint16_t tid = (msg[0] << 8) | msg[1];

		if (tid_ == tid) 
			return Protocol::ParseSlaveMsg(msg + kHdrSiz, msglen - kHdrSiz, inf);
		
		return -EBADMSG;
	}

private:
	const uint8_t kHdrSiz = 6;
	uint16_t tid_;
};

template<typename TBase>
const char* Net<TBase>::protname = "NET";

} //namespace YModbus

#endif // ! __YMODBUS_YMBNET_H__

