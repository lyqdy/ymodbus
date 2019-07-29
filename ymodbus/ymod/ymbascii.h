/**
* ymodbus
* Copyright © 2019-2019 liuyongqing<lyqdy1@163.com>
* v1.0.1 2019.05.04
*/
#ifndef __YMODBUS_YMBASCII_H__
#define __YMODBUS_YMBASCII_H__

#include "ymod/ymbprot.h"
#include "ymod/ymbdefs.h"
#include "ymblog.h"

#include <memory>
#include <algorithm>
#include <numeric>

namespace YModbus {

template<typename TBase>
class Ascii : public TBase
{
public:
	static const char *protname;

	int GetMasterDataOffset(uint8_t fun)
	{
		return static_cast<int>(Protocol::GetMasterDataOffset(fun) + kRtuBufOff);
	}

	int GetSlaveDataOffset(uint8_t fun)
	{
		return static_cast<int>(Protocol::GetSlaveDataOffset(fun) + kRtuBufOff);
	}

	size_t MakeMasterMsg(uint8_t *buf, size_t bufsiz, MsgInf &inf)
	{
		YMB_ASSERT(bufsiz >= kMaxAsciiMsgLen);

		uint8_t *pbuf = buf;
		uint8_t *prtu = buf + kRtuBufOff;
		size_t msglen = Protocol::MakeMasterMsg(prtu, bufsiz-kRtuBufOff, inf);
		uint8_t lrc = (uint8_t)(~std::accumulate(prtu, prtu + msglen, 0) + 1);

		*pbuf++ = ':'; //start char
		std::for_each(prtu, prtu + msglen, [&pbuf](uint8_t d) {
			*pbuf = static_cast<uint8_t>(d >> 4);
			*pbuf += *pbuf <= 9 ? '0' : 'A' - 0xA;
			pbuf++;
			*pbuf = static_cast<uint8_t>(d & 0xf);
			*pbuf += *pbuf <= 9 ? '0' : 'A' - 0xA;
			pbuf++;
		});

		*pbuf = static_cast<uint8_t>(lrc >> 4);
		*pbuf += *pbuf <= 9 ? '0' : 'A' - 0xA;
		pbuf++;
		*pbuf = static_cast<uint8_t>(lrc & 0xf);
		*pbuf += *pbuf <= 9 ? '0' : 'A' - 0xA;
		pbuf++;

		*pbuf++ = '\r';
		*pbuf++ = '\n';

		msglen = static_cast<size_t>(pbuf - buf);
		YMB_ASSERT(bufsiz >= msglen);

		return msglen;
	}

	size_t MakeSlaveMsg(uint8_t *buf, size_t bufsiz, MsgInf &inf)
	{
		YMB_ASSERT(bufsiz >= kMaxAsciiMsgLen);

		uint8_t *pbuf = buf;
		uint8_t *prtu = buf + kRtuBufOff;
		size_t msglen = Protocol::MakeSlaveMsg(prtu, bufsiz - kRtuBufOff, inf);
		uint8_t lrc = (uint8_t)(~std::accumulate(prtu, prtu + msglen, 0) + 1);

		*pbuf++ = ':'; //start char
		std::for_each(prtu, prtu + msglen, [&pbuf](uint8_t d) {
			*pbuf = static_cast<uint8_t>(d >> 4);
			*pbuf += *pbuf <= 9 ? '0' : 'A' - 0xA;
			pbuf++;
			*pbuf = static_cast<uint8_t>(d & 0xf);
			*pbuf += *pbuf <= 9 ? '0' : 'A' - 0xA;
			pbuf++;
		});

		*pbuf = static_cast<uint8_t>(lrc >> 4);
		*pbuf += *pbuf <= 9 ? '0' : 'A' - 0xA;
		pbuf++;
		*pbuf = static_cast<uint8_t>(lrc & 0xf);
		*pbuf += *pbuf <= 9 ? '0' : 'A' - 0xA;
		pbuf++;

		*pbuf++ = '\r';
		*pbuf++ = '\n';

		msglen = static_cast<size_t>(pbuf - buf);
		YMB_ASSERT(bufsiz >= msglen);

		return msglen;
	}

	//Exact message
	int VerifyMasterMsg(uint8_t *msg, size_t msglen)
	{
		if (msglen < kMinAsciiMsgLen) //:-id-fun-xxx-lrc-\r\n
			return static_cast<int>(kMinAsciiMsgLen - msglen); //expect data

		if (msg[0] != ':')
			return -EBADMSG;

		if (msglen > kMaxAsciiMsgLen)
			return -EBADMSG;

		if (msg[msglen - 2] != '\r')
			return 2;	//expect '\r\n'

		if (msg[msglen - 1] != '\n')
			return -EBADMSG;	//must be '\n'

		return EOK;
	}

	int VerifySlaveMsg(uint8_t *msg, size_t msglen)
	{
		if (msglen < kMinAsciiMsgLen) //:-id-fun-err-lrc-\r\n
			return static_cast<int>(kMinAsciiMsgLen - msglen); //expect data

		if (msg[0] != ':')
			return -EBADMSG;

		if (msglen > kMaxAsciiMsgLen)
			return -EBADMSG;

		if (msg[msglen - 2] != '\r')
			return 2;	//expect '\r\n'

		if (msg[msglen - 1] != '\n')
			return -EBADMSG;	//must be '\n'

		return EOK;
	}

	//Used by slave
	int ParseMasterMsg(uint8_t *msg, size_t msglen, MsgInf &inf)
	{
		//Transform to base format msg
		uint8_t *prtu = msg;
		size_t i;

		for (i = 1; i < msglen - 4; i += 2) {
			*prtu = msg[i] <= '9' ? msg[i] - '0' : msg[i] - 'A' + 0xA;
			*prtu <<= 4;
			*prtu |= msg[i+1] <= '9' ? msg[i+1] - '0' : msg[i+1] - 'A' + 0xA;
			++prtu;
		}

		//msg has transformed base format
		size_t rtulen = static_cast<size_t>(prtu - msg);
		uint8_t lrc = (uint8_t)(~std::accumulate(msg, msg + rtulen, 0) + 1);

		lrc -= (msg[i] <= '9' ? msg[i] - '0' : msg[i] - 'A' + 0xA) << 4;
		lrc -= msg[i + 1] <= '9' ? msg[i + 1] - '0' : msg[i + 1] - 'A' + 0xA;

		if (lrc == 0 && Protocol::VerifyMasterMsg(msg, rtulen) == EOK)
			return Protocol::ParseMasterMsg(msg, rtulen, inf);

		return -EBADMSG;
	}

	//Used by master
	int ParseSlaveMsg(uint8_t *msg, size_t msglen, MsgInf &inf)
	{
		//Transform to base format msg
		uint8_t *prtu = msg;
		size_t i;

		for (i = 1; i < msglen - 4; i += 2) {
			*prtu = msg[i] <= '9' ? msg[i] - '0' : msg[i] - 'A' + 0xA;
			*prtu <<= 4;
			*prtu |= msg[i+1] <= '9' ? msg[i+1] - '0' : msg[i+1] - 'A' + 0xA;
			++prtu;
		}

		//msg has transformed base format
		size_t rtulen = static_cast<size_t>(prtu - msg);
		uint8_t lrc = (uint8_t)(~std::accumulate(msg, msg + rtulen, 0) + 1);

		lrc -= (msg[i] <= '9' ? msg[i] - '0' : msg[i] - 'A' + 0xA) << 4;
		lrc -= msg[i + 1] <= '9' ? msg[i + 1] - '0' : msg[i + 1] - 'A' + 0xA;

		if (lrc == 0 && Protocol::VerifySlaveMsg(msg, rtulen) == EOK)
			return Protocol::ParseSlaveMsg(msg, rtulen, inf);

		return -EBADMSG;
	}

private:
	const size_t kMinAsciiMsgLen = 8;
	const size_t kMaxAsciiMsgLen = 513;
	const size_t kMaxRtuMsgLen = 254;
	const size_t kRtuBufOff = 513 - 254 - 1;
};

template<typename TBase>
const char* Ascii<TBase>::protname = "ASCII";

} //namespace YModbus

#endif // ! __YMODBUS_YMBASCII_H__

