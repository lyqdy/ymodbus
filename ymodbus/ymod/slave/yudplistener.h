/**
* ymodbus
* Copyright © 2019-2019 liuyongqing<lyqdy1@163.com>
* v1.0.1 2019.05.04
*/
#ifndef __YMODBUS_YUDPLISTENER_H__
#define __YMODBUS_YUDPLISTENER_H__

#include "ymod/slave/ylistener.h"

#include <memory>

namespace YModbus {

class UdpListener : public IListerner
{
public:
	UdpListener(uint16_t port);
	~UdpListener();

	std::string GetName(void);
	void SetTimeout(long to);
	bool Listen(void);
	int Accept(std::vector<SessionPtr> &ses);

private:
	struct Impl;
	std::shared_ptr<Impl> impl_;
};

} //namespace YModbus

#endif // ! __YMODBUS_YUDPLISTENER_H__

