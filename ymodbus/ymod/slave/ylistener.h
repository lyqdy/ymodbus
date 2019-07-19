/**
* ymodbus
* Copyright © 2019-2019 liuyongqing<lyqdy1@163.com>
* v1.0.1 2019.05.04
*/
#ifndef __YMODBUS_YLISTENER_H__
#define __YMODBUS_YLISTENER_H__

#include "ymod/slave/ymbsession.h"

#include <vector>

namespace YModbus {

class IListerner
{
public:
	virtual std::string GetName(void) = 0;
	virtual void SetTimeout(long to) = 0;
	virtual bool Listen(void) = 0;
	virtual int Accept(std::vector<SessionPtr> &ses) = 0;
	virtual ~IListerner() {}
};

} //namespace YModbus

#endif // ! __YMODBUS_YLISTENER_H__

