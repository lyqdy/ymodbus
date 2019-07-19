/**
* ymodbus
* Copyright © 2019-2019 liuyongqing<lyqdy1@163.com>
* v1.0.1 2019.05.04
*/
#ifndef __YMODBUS_MBCONNECT_H__
#define __YMODBUS_MBCONNECT_H__

#include <stdint.h>

namespace YModbus {

class IConnect
{
public:
	virtual void SetTimeout(long to) = 0;
	virtual bool Validate(void) = 0;
	virtual void Purge(void) = 0;

	virtual bool Send(uint8_t  *buf, size_t len) = 0;
	virtual int Recv(uint8_t *buf, size_t len) = 0;

	virtual ~IConnect() {}
};

} //namespace YModbus

#endif //__YMODBUS_MBCONNECT_H__

