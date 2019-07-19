/**
* ymodbus
* Copyright © 2019-2019 liuyongqing<lyqdy1@163.com>
* v1.0.1 2019.05.04
*/
#ifndef __YMODBUS_YSERCONNECT_H__
#define __YMODBUS_YSERCONNECT_H__

#include "ymod/master/yconnect.h"

#include <memory>
#include <string>

namespace YModbus {

class SerConnect : public IConnect
{
public:
	SerConnect(const std::string &com, 
		uint32_t baudrate, uint8_t databits, char parity, uint8_t stopbits);
	~SerConnect();

	void SetTimeout(long to);
	bool Validate(void);
	void Purge(void);

	bool Send(uint8_t  *buf, size_t len);
	int Recv(uint8_t *buf, size_t len);

private:
	struct Impl;
	std::unique_ptr<Impl> impl_;
};

} //namespace YModbus

#endif //__YMODBUS_YSERCONNECT_H__
