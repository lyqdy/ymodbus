/**
* ymodbus
* Copyright © 2019-2019 liuyongqing<lyqdy1@163.com>
* v1.0.1 2019.08.11
*/
#ifndef __YMODBUS_YMBMONITOR_H__
#define __YMODBUS_YMBMONITOR_H__

#include <cstdint>
#include <string>

namespace YModbus {

//数据为大端字节序
class IMonitor
{
public:
	virtual ~IMonitor() {}
	virtual void SendPacket(const std::string &desc, const uint8_t *val, int len) = 0;
	virtual void RecvPacket(const std::string &desc, const uint8_t *val, int len) = 0;
};

} //namespace YModbus

#endif //__YMODBUS_YMBMONITOR_H__
