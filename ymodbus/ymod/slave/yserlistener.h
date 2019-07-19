/**
* ymodbus
* Copyright © 2019-2019 liuyongqing<lyqdy1@163.com>
* v1.0.1 2019.05.04
*/
#ifndef __YMODBUS_YSERLISTENER_H__
#define __YMODBUS_YSERLISTENER_H__

#include "ymod/slave/ylistener.h"

#include <memory>

namespace YModbus {

class SerListener : public IListerner
{
public:
	SerListener(const std::string& port,
		uint32_t baudrate,
		uint8_t databits,
		char parity,
		uint8_t stopbits);
	~SerListener();

	std::string GetName(void);
	void SetTimeout(long to);
	bool Listen(void);
	int Accept(std::vector<SessionPtr> &ses);

private:
	struct Impl;
	std::shared_ptr<Impl> impl_; //for SessionPtr, shared_ptr is used.
};

} //namespace YModbus

#endif // ! __YMODBUS_YSERLISTENER_H__

