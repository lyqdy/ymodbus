/**
* ymodbus
* Copyright © 2019-2019 liuyongqing<lyqdy1@163.com>
* v1.0.1 2019.05.04
*/
#include "ymod/ymbprot.h"
#include "ymod/ymbdefs.h"
#include "ymblog.h"

#include <memory>
#include <cstring>

namespace YModbus {

uint8_t GetMasterMsgMinLen(uint8_t *msg)
{
	switch (msg[1]) {
	case kFunReadCoils:
	case kFunReadDiscreteInputs:
	case kFunReadHoldingRegisters:
	case kFunReadInputRegisters:
		return 6;
	case kFunWriteAndReadRegisters:
		return 10 + 1;
	case kFunWriteMultiCoils:
	case kFunWriteMultiRegisters:
		return 6 + 1;
	case kFunWriteSingleCoil:
	case kFunWriteSingleRegister:
		return 6;
	case kFunMaskWriteRegister:
		return 8;
	default:
		return 2;
	}
}

uint8_t GetSlaveMsgMinLen(uint8_t *msg)
{
	switch (msg[1]) {
	case kFunReadCoils:
	case kFunReadDiscreteInputs:
	case kFunReadHoldingRegisters:
	case kFunReadInputRegisters:
		return 3;
	case kFunWriteAndReadRegisters:
		return 3;
	case kFunWriteMultiCoils:
	case kFunWriteMultiRegisters:
		return 6;
	case kFunWriteSingleCoil:
	case kFunWriteSingleRegister:
		return 6;
	case kFunMaskWriteRegister:
		return 8;
	default:
		return 2;
	}
}

uint8_t GetMasterMsgMaxLen(uint8_t *msg)
{
	switch (msg[1]) {
	case kFunReadCoils:
	case kFunReadDiscreteInputs:
	case kFunReadHoldingRegisters:
	case kFunReadInputRegisters:
		return 6;
	case kFunWriteAndReadRegisters:
		return 10 + 1 + msg[10];//id-fun-rn-ru-wa-wn-wb
	case kFunWriteMultiCoils:
	case kFunWriteMultiRegisters:
		return 6 + 1 + msg[6];//id-fun-wa-wn-wb
	case kFunWriteSingleCoil:
	case kFunWriteSingleRegister:
		return 6;
	case kFunMaskWriteRegister:
		return 8;
	default:
		return 255;
	}
}

uint8_t GetSlaveMsgMaxLen(uint8_t *msg)
{
	switch (msg[1]) {
	case kFunReadCoils:
	case kFunReadDiscreteInputs:
	case kFunReadHoldingRegisters:
	case kFunReadInputRegisters:
		return 3 + msg[2];
	case kFunWriteAndReadRegisters:
		return 3 + msg[2];
	case kFunWriteMultiCoils:
	case kFunWriteMultiRegisters:
		return 6;
	case kFunWriteSingleCoil:
	case kFunWriteSingleRegister:
		return 6;
	case kFunMaskWriteRegister:
		return 8;
	default:
		return 255;
	}
}

//direct: 1: in, /master/request, 0: out, slave/response
//return: offset from first byte of local format msg based 0
int Protocol::GetMasterDataOffset(uint8_t fun)
{
	switch (fun) {
	case kFunReadCoils:
	case kFunReadDiscreteInputs:
	case kFunReadHoldingRegisters:
	case kFunReadInputRegisters:
		return 6;//id-fun-rreg-rnum
	case kFunWriteMultiCoils:
	case kFunWriteMultiRegisters:
		return 6 + 1;//id-fun-rreg-rnum-bytes
	case kFunWriteAndReadRegisters:
		return 10 + 1;//id-fun-rreg-rnum-wreg-wnum-bytes
	case kFunWriteSingleCoil:
	case kFunWriteSingleRegister:
	case kFunMaskWriteRegister:
		return 4;//id-fun-reg
	default:
		return 2;
	}
}

//direct: 1: in, /master/request, 0: out, slave/response
//return: offset from first byte of local format msg based 0
int Protocol::GetSlaveDataOffset(uint8_t fun)
{
	switch (fun) {
	case kFunReadCoils:
	case kFunReadDiscreteInputs:
	case kFunReadHoldingRegisters:
	case kFunReadInputRegisters:
	case kFunWriteAndReadRegisters:
		return 3;//id-fun-bytes
	case kFunWriteMultiCoils:
	case kFunWriteMultiRegisters:
		return 6;//id-fun-reg-num
	case kFunWriteSingleCoil:
	case kFunWriteSingleRegister:
	case kFunMaskWriteRegister:
		return 4; //id-fun-reg
	default:
		return 2;
	}
}

size_t Protocol::MakeMasterMsg(uint8_t *buf, size_t bufsiz, MsgInf &inf)
{
	uint8_t *pbuf = buf;
	
	*pbuf++ = inf.id;
	*pbuf++ = inf.fun;
	
	switch (inf.fun) {
	case kFunWriteSingleCoil:
	case kFunWriteSingleRegister:
	case kFunMaskWriteRegister:
		*pbuf++ = static_cast<uint8_t>(inf.wreg >> 8);
		*pbuf++ = static_cast<uint8_t>(inf.wreg & 0xff);
		break;
	case kFunWriteMultiCoils:
	case kFunWriteMultiRegisters:
		*pbuf++ = static_cast<uint8_t>(inf.wreg >> 8);
		*pbuf++ = static_cast<uint8_t>(inf.wreg & 0xff);
		*pbuf++ = static_cast<uint8_t>(inf.wnum >> 8);
		*pbuf++ = static_cast<uint8_t>(inf.wnum & 0xff);
		*pbuf++ = static_cast<uint8_t>(inf.datalen);
		break;
	case kFunReadCoils:
	case kFunReadDiscreteInputs:
	case kFunReadHoldingRegisters:
	case kFunReadInputRegisters:
		*pbuf++ = static_cast<uint8_t>(inf.rreg >> 8);
		*pbuf++ = static_cast<uint8_t>(inf.rreg & 0xff);
		*pbuf++ = static_cast<uint8_t>(inf.rnum >> 8);
		*pbuf++ = static_cast<uint8_t>(inf.rnum & 0xff);
		break;
	case kFunWriteAndReadRegisters:
		*pbuf++ = static_cast<uint8_t>(inf.rreg >> 8);
		*pbuf++ = static_cast<uint8_t>(inf.rreg & 0xff);
		*pbuf++ = static_cast<uint8_t>(inf.rnum >> 8);
		*pbuf++ = static_cast<uint8_t>(inf.rnum & 0xff);
		*pbuf++ = static_cast<uint8_t>(inf.wreg >> 8);
		*pbuf++ = static_cast<uint8_t>(inf.wreg & 0xff);
		*pbuf++ = static_cast<uint8_t>(inf.wnum >> 8);
		*pbuf++ = static_cast<uint8_t>(inf.wnum & 0xff);
		*pbuf++ = static_cast<uint8_t>(inf.datalen);
		break;
	default:
		YMB_ASSERT(false && "Function is not surpported");
		break;
	}

	//data maybe has been copied, in this case, databuf is null.
	if (inf.databuf != nullptr) { //write or read data
		memmove(pbuf, inf.databuf, inf.datalen);
	}
	pbuf += inf.datalen; //datalen must always is valid!

	size_t msglen = static_cast<size_t>(pbuf - buf);
	YMB_ASSERT(bufsiz >=  msglen);

	return msglen;
}

size_t Protocol::MakeSlaveMsg(uint8_t *buf, size_t bufsiz, MsgInf &inf)
{
	uint8_t *pbuf = buf;

	*pbuf++ = inf.id;

	if (inf.err == 0) {
		*pbuf++ = inf.fun;
		switch (inf.fun) {
		case kFunWriteSingleCoil:
		case kFunWriteSingleRegister:
		case kFunMaskWriteRegister:
			*pbuf++ = static_cast<uint8_t>(inf.wreg >> 8);
			*pbuf++ = static_cast<uint8_t>(inf.wreg & 0xff);
			break;
		case kFunWriteMultiCoils:
		case kFunWriteMultiRegisters:
			*pbuf++ = static_cast<uint8_t>(inf.wreg >> 8);
			*pbuf++ = static_cast<uint8_t>(inf.wreg & 0xff);
			*pbuf++ = static_cast<uint8_t>(inf.wnum >> 8);
			*pbuf++ = static_cast<uint8_t>(inf.wnum & 0xff);
			break;
		case kFunReadCoils:
		case kFunReadDiscreteInputs:
		case kFunReadHoldingRegisters:
		case kFunReadInputRegisters:
		case kFunWriteAndReadRegisters:
			*pbuf++ = static_cast<uint8_t>(inf.datalen);
			break;
		default:
			YMB_ASSERT(false);
			break;
		}
	}
	else {
		*pbuf++ = inf.fun | 0x80;
		*pbuf++ = inf.err;
	}

	//data maybe has been copied, in this case, databuf is null.
	if (inf.databuf != nullptr) {	//write or read data
		memmove(pbuf, inf.databuf, inf.datalen);
	}
	pbuf += inf.datalen;	//datalen must always is valid!

	size_t msglen = static_cast<size_t>(pbuf - buf);
	YMB_ASSERT(bufsiz >= msglen);

	return msglen;
}
	
//Exact message
int Protocol::VerifyMasterMsg(uint8_t *msg, size_t msglen)
{
	size_t len = 3; //id-fun-err
	if (msglen < len)
		return static_cast<int>(len - msglen);

	if (msg[1] & 0x80)
		return EOK; //exception msg

	len = GetMasterMsgMinLen(msg);
	if (msglen < len)
		return static_cast<int>(len - msglen);

	len = GetMasterMsgMaxLen(msg);
	if (msglen < len)
		return static_cast<int>(len - msglen);

	return EOK; 
}

int Protocol::VerifySlaveMsg(uint8_t *msg, size_t msglen)
{
	size_t len = 3; //id-fun-err
	if (msglen < len)
		return static_cast<int>(len - msglen);

	if (msg[1] & 0x80)
		return EOK; //exception msg

	len = GetSlaveMsgMinLen(msg);
	if (msglen < len)
		return static_cast<int>(len - msglen);

	len = GetSlaveMsgMaxLen(msg);
	if (msglen < len)
		return static_cast<int>(len - msglen);

	return EOK; 
}

//Used by slave
int Protocol::ParseMasterMsg(uint8_t *msg, size_t msglen, MsgInf &inf)
{
	uint8_t *pbuf = msg;

	inf.id = *pbuf++;
	inf.fun = *pbuf++;

	switch (inf.fun) {
	case kFunWriteSingleCoil:
	case kFunWriteSingleRegister:
	case kFunMaskWriteRegister:
		inf.rreg = INVALID_REG;
		inf.rnum = INVALID_NUM;
		inf.wreg = *pbuf++ << 8;
		inf.wreg |= *pbuf++;
		inf.wnum = 1;
		inf.datalen = 2;
		inf.databuf = pbuf;
		break;
	case kFunWriteMultiCoils:
	case kFunWriteMultiRegisters:
		inf.rreg = INVALID_REG;
		inf.rnum = INVALID_NUM;
		inf.wreg = *pbuf++ << 8;
		inf.wreg |= *pbuf++;
		inf.wnum = *pbuf++ << 8;
		inf.wnum |= *pbuf++;
		inf.datalen = *pbuf++;
		inf.databuf = pbuf;
		break;
	case kFunReadCoils:
	case kFunReadDiscreteInputs:
	case kFunReadHoldingRegisters:
	case kFunReadInputRegisters:
		inf.wreg = INVALID_REG;
		inf.wnum = INVALID_NUM;
		inf.rreg = *pbuf++ << 8;
		inf.rreg |= *pbuf++;
		inf.rnum = *pbuf++ << 8;
		inf.rnum |= *pbuf++;
		inf.datalen = 0;
		inf.databuf = nullptr;
		break;
	case kFunWriteAndReadRegisters:
		inf.rreg = *pbuf++ << 8;
		inf.rreg |= *pbuf++;
		inf.rnum = *pbuf++ << 8;
		inf.rnum |= *pbuf++;
		inf.wreg = *pbuf++ << 8;
		inf.wreg |= *pbuf++;
		inf.wnum = *pbuf++ << 8;
		inf.wnum |= *pbuf++;
		inf.datalen = *pbuf++;
		inf.databuf = pbuf;
		break;
	default:
		YMB_ERROR("Modbus function not surpport. fun = %u\n", inf.fun);
		return -EBADMSG;
	}

	YMB_HEXDUMP(msg, msglen, 
		"Master message: id = 0x%02x, fun = 0x%02x: ", inf.id, inf.fun);

	return EOK;
}

//Used by master
int Protocol::ParseSlaveMsg(uint8_t *msg, size_t /*msglen*/, MsgInf &inf)
{
	uint8_t *pbuf = msg;

	inf.id = *pbuf++;
	inf.fun = *pbuf++;

	if ((inf.fun & 0x80) == 0) {
		switch (inf.fun) {
		case kFunWriteSingleCoil:
		case kFunWriteSingleRegister:
		case kFunMaskWriteRegister:
			inf.rreg = INVALID_REG;
			inf.rnum = INVALID_NUM;
			inf.wreg = *pbuf++ << 8;
			inf.wreg |= *pbuf++;
			inf.wnum = 1;
			inf.datalen = 2;
			inf.databuf = pbuf;
			break;
		case kFunWriteMultiCoils:
		case kFunWriteMultiRegisters:
			inf.rreg = INVALID_REG;
			inf.rnum = INVALID_NUM;
			inf.wreg = *pbuf++ << 8;
			inf.wreg |= *pbuf++;
			inf.wnum = *pbuf++ << 8;
			inf.wnum |= *pbuf++;
			inf.datalen = 0;
			inf.databuf = nullptr;
			break;
		case kFunReadCoils:
		case kFunReadDiscreteInputs:
		case kFunReadHoldingRegisters:
		case kFunReadInputRegisters:
			inf.datalen = *pbuf++;
			inf.databuf = pbuf;
			break;
		case kFunWriteAndReadRegisters:
			inf.datalen = *pbuf++;
			inf.databuf = pbuf;
			break;
		default:
			return -EBADMSG;
		}
	}
	else { //exception
		inf.err = *pbuf++;
	}

	YMB_HEXDUMP0(msg, msglen,
		"Slave message: id = 0x%02x, fun = 0x%02x: ", inf.id, inf.fun);

	return EOK;
}

} //namespace YModbus

