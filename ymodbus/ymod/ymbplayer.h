/**
* ymodbus
* Copyright © 2019-2019 liuyongqing<lyqdy1@163.com>
* v1.0.1 2019.05.04
*/
#ifndef __YMODBUS_YMBPLAYER_H__
#define __YMODBUS_YMBPLAYER_H__

#include <cstdint>
#include <cstddef>

namespace YModbus {

class IPlayer
{
public:
	//return: >= 0, bytes of data to return;
	//return: < 0, errorcode of exception
	//buf: data value, net order
	virtual int ReadCoils(uint8_t sid,
		uint16_t reg, uint16_t num, uint8_t *buf, size_t bufsiz) = 0;
	virtual int ReadDiscreteInputs(uint8_t sid,
		uint16_t reg, uint16_t num, uint8_t *buf, size_t bufsiz) = 0;
	virtual int ReadInputRegisters(uint8_t sid,
		uint16_t reg, uint16_t num, uint8_t *buf, size_t bufsiz) = 0;
	virtual int ReadHoldingRegisters(uint8_t sid,
		uint16_t reg, uint16_t num, uint8_t *buf, size_t bufsiz) = 0;

	//return: = 0, OK;
	//return: < 0, errorcode of exception
	//values: data value, net order
	virtual int WriteSingleCoil(uint8_t sid, uint16_t reg, bool onoff) = 0;
	virtual int WriteCoils(uint8_t sid,
		uint16_t reg, uint16_t num, const uint8_t *bits, uint8_t wbytes) = 0;
	virtual int WriteSingleRegister(uint8_t sid,
		uint16_t reg, uint16_t value) = 0;
	virtual int WriteRegisters(uint8_t sid,
		uint16_t reg, uint16_t num, const uint8_t *values, uint8_t wbytes) = 0;
	virtual int MaskWriteRegisters(uint8_t sid,
		uint16_t reg, uint16_t andmask, uint16_t ormask) = 0;

	//return: >= 0, bytes of data to return
	//return: < 0,  errorcode of exception
	//values/buf: data value, net order
	virtual int WriteReadRegisters(uint8_t sid,
		uint16_t wreg, uint16_t wnum, const uint8_t *values, uint8_t wbytes,
		uint16_t rreg, uint16_t rnum, uint8_t *buf, size_t bufsiz) = 0;

	//return: >= 0, OK
	//return: < 0,  errorcode of exception
	virtual int ReportSlaveId(uint8_t maxsid, uint8_t *buf, size_t bufsiz) = 0;

	virtual ~IPlayer() {}
};

} //namespace YModbus

#endif // !__YMODBUS_YMBPLAYER_H__
