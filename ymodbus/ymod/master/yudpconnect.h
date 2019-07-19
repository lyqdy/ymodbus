/**
* ymodbus
* Copyright © 2019-2019 liuyongqing<lyqdy1@163.com>
* v1.0.1 2019.05.04
*/
#ifndef __YMODBUS_YUDPCONNECT_H__
#define __YMODBUS_YUDPCONNECT_H__

#include "ymod/master/yconnect.h"

#include <memory>
#include <string>

namespace YModbus {

class UdpConnect : public IConnect
{
public:
	UdpConnect(const std::string &ip, uint16_t port);
	~UdpConnect();

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

#endif //__YMODBUS_YUDPCONNECT_H__
