/**
* ymodbus
* Copyright © 2019-2019 liuyongqing<lyqdy1@163.com>
* v1.0.1 2019.05.04
*/
#ifndef __YMODBUS_YPROTOCOL_H__
#define __YMODBUS_YPROTOCOL_H__

#include <cstdint>
#include <cstddef>
#include <memory>

namespace YModbus {

struct MsgInf {
	MsgInf()
	{ 
	}

	MsgInf(uint8_t i,
		uint8_t f,
		uint16_t r,
		uint16_t n)
		: id(i)
		, fun(f)
		, rreg(r)
		, rnum(n)
		, wreg(0)
		, wnum(0)
		, databuf(nullptr)
		, datalen(0)
		, pbuf(nullptr)
		, bufsiz(0)
		, err(0)
	{
	}

	MsgInf(uint8_t i,
		uint8_t f,
		uint16_t r,
		uint16_t n,
		uint16_t wr,
		uint16_t wn)
		: id(i)
		, fun(f)
		, rreg(r)
		, rnum(n)
		, wreg(wr)
		, wnum(wn)
		, databuf(nullptr)
		, datalen(0)
		, pbuf(nullptr)
		, bufsiz(0)
		, err(0)
	{
	}

	MsgInf(uint8_t i,
		uint8_t f,
		uint16_t r,
		uint16_t n,
		uint16_t wr,
		uint16_t wn,
		uint8_t *d,
		uint8_t l)
		: id(i)
		, fun(f)
		, rreg(r)
		, rnum(n)
		, wreg(wr)
		, wnum(wn)
		, databuf(d)
		, datalen(l)
		, pbuf(nullptr)
		, bufsiz(0)
		, err(0)
	{
	}

	uint8_t id;
	uint8_t fun;
	uint16_t rreg;
	uint16_t rnum;
	uint16_t wreg;
	uint16_t wnum;
	uint8_t *databuf;
	uint8_t datalen;
	uint8_t *pbuf;
	size_t bufsiz;
	uint8_t err;
};

class IProtocol
{
public:
	//Master-----------------------------------------------------------------
	//direct: 1: in, request, 0: out, response
	virtual int GetMasterDataOffset(uint8_t fun) = 0;

	//Used by master
	virtual size_t MakeMasterMsg(uint8_t *buf, size_t bufsiz, MsgInf &inf) = 0;

	//Used by slave
	//If msg OK, return 0, else return bytes of data expected
	virtual int VerifyMasterMsg(uint8_t *msg, size_t msglen) = 0;

	//Used by slave
	virtual int ParseMasterMsg(uint8_t *msg, size_t msglen, MsgInf &inf) = 0;

	//Slave------------------------------------------------------------------
	//direct: 1: in, request, 0: out, response
	virtual int GetSlaveDataOffset(uint8_t fun) = 0;

	//Used by slave
	virtual size_t MakeSlaveMsg(uint8_t *buf, size_t bufsiz, MsgInf &inf) = 0;

	//Used by master
	//If msg OK, return 0, else return bytes of data expected
	//tid_ will be checked...
	virtual int VerifySlaveMsg(uint8_t *msg, size_t msglen) = 0;

	//Used by master
	virtual int ParseSlaveMsg(uint8_t *msg, size_t msglen, MsgInf &inf) = 0;

	~IProtocol() {}
};

class Protocol
{
public:
	//Master-----------------------------------------------------------------
	//direct: 1: in, request, 0: out, response
	static int GetMasterDataOffset(uint8_t fun);

	//Used by master
	static size_t MakeMasterMsg(uint8_t *buf, size_t bufsiz, MsgInf &inf);
	
	//Used by slave
	//If msg OK, return 0, else return bytes of data expected
	static int VerifyMasterMsg(uint8_t *msg, size_t msglen);
	
	//Used by slave
	static int ParseMasterMsg(uint8_t *msg, size_t msglen, MsgInf &inf);
	
	//Slave------------------------------------------------------------------
	//direct: 1: in, request, 0: out, response
	static int GetSlaveDataOffset(uint8_t fun);

	//Used by slave
	static size_t MakeSlaveMsg(uint8_t *buf, size_t bufsiz, MsgInf &inf);
	
	//Used by master
	//If msg OK, return 0, else return bytes of data expected
	//tid_ will be checked...
	static int VerifySlaveMsg(uint8_t *msg, size_t msglen);

	//Used by master
	static int ParseSlaveMsg(uint8_t *msg, size_t msglen, MsgInf &inf);
};

} //namespace YModbus

#endif // ! __YMODBUS_YPROTOCOL_H__

