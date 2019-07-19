/**
* ymodbus
* Copyright © 2019-2019 liuyongqing<lyqdy1@163.com>
* v1.0.1 2019.05.04
*/
#ifndef __YMODBUS_YMBSESSION_H__
#define __YMODBUS_YMBSESSION_H__

#include <string>
#include <memory>

namespace YModbus {

class ISession
{
public:
	virtual std::string PeerName(void) = 0;
	virtual int Write(uint8_t *msg, size_t msglen) = 0;
	virtual int Read(uint8_t *buf, size_t bufsiz) = 0;
	virtual size_t Peek(uint8_t **buf) = 0;

	virtual void Purge(void) = 0;
	virtual void Discard(size_t nbytes) = 0;

	virtual ~ISession() {}
};

typedef std::shared_ptr<ISession> SessionPtr;

} //namespace YModbus

#endif // ! __YMODBUS_YMBSESSION_H__

