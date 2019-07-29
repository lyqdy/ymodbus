/**
* ymodbus
* Copyright © 2019-2019 liuyongqing<lyqdy1@163.com>
* v1.0.1 2019.05.04
*/
#ifndef __YMODBUS_YMBRTU_H__
#define __YMODBUS_YMBRTU_H__

#include "ymod/ymbprot.h"
#include "ymod/ymbdefs.h"
#include "ymod/ymbcrc.h"
#include "ymblog.h"

namespace YModbus {

template<typename TBase>
class Rtu : public TBase
{
public:
	static const char *protname;

	int GetMasterDataOffset(uint8_t fun)
	{
		return Protocol::GetMasterDataOffset(fun);
	}

	int GetSlaveDataOffset(uint8_t fun)
	{
		return Protocol::GetSlaveDataOffset(fun);
	}

	size_t MakeMasterMsg(uint8_t *buf, size_t bufsiz, MsgInf &inf)
	{
		size_t msglen = Protocol::MakeMasterMsg(buf, bufsiz - 2, inf);
		YMB_ASSERT(bufsiz >= static_cast<size_t>(msglen + 2));

		uint16_t crc = Crc16(buf, static_cast<uint16_t>(msglen));

		buf[msglen + 0] = static_cast<uint8_t>(crc & 0xff);
		buf[msglen + 1] = static_cast<uint8_t>(crc >> 8);

		return msglen + 2;
	}

	size_t MakeSlaveMsg(uint8_t *buf, size_t bufsiz, MsgInf &inf)
	{
		size_t msglen = Protocol::MakeSlaveMsg(buf, bufsiz - 2, inf);
		YMB_ASSERT(bufsiz >= msglen + 2);

		uint16_t crc = Crc16(buf, static_cast<uint16_t>(msglen));

		buf[msglen + 0] = static_cast<uint8_t>(crc & 0xff);
		buf[msglen + 1] = static_cast<uint8_t>(crc >> 8);

		return msglen + 2;
	}

	//Exact message
	int VerifyMasterMsg(uint8_t *msg, size_t msglen)
	{
		if (msglen < kMinRtuMsgLen)
			return static_cast<int>(kMinRtuMsgLen - msglen);

		int expect = Protocol::VerifyMasterMsg(msg, msglen - 2);
		if (expect != 0)
			return expect;

		uint16_t crc = Crc16(msg, static_cast<uint16_t>(msglen - 2));

		return crc == ((msg[msglen - 1] << 8) | msg[msglen - 2]) ? 0 : -EVAL;
	}

	int VerifySlaveMsg(uint8_t *msg, size_t msglen)
	{
		if (msglen < kMinRtuMsgLen)
			return static_cast<int>(kMinRtuMsgLen - msglen);

		int expect = Protocol::VerifySlaveMsg(msg, msglen - 2);
		if (expect != 0)
			return expect;

		uint16_t crc = Crc16(msg, static_cast<uint16_t>(msglen - 2));

		return crc == ((msg[msglen - 1] << 8) | msg[msglen - 2]) ? 0 : -EVAL;
	}

	//Used by slave
	int ParseMasterMsg(uint8_t *msg, size_t msglen, MsgInf &inf)
	{
		YMB_ASSERT(msglen >= kMinRtuMsgLen);
		return Protocol::ParseMasterMsg(msg, msglen - 2, inf);
	}

	//Used by master
	int ParseSlaveMsg(uint8_t *msg, size_t msglen, MsgInf &inf)
	{
		YMB_ASSERT(msglen >= kMinRtuMsgLen);
		return Protocol::ParseSlaveMsg(msg, msglen - 2, inf);
	}

private:
	const size_t kMinRtuMsgLen = 5;
};

template<typename TBase>
const char* Rtu<TBase>::protname = "RTU";

} //namespace YModbus

#endif // ! __YMODBUS_YMBRTU_H__

