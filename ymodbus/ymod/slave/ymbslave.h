/**
* ymodbus
* Copyright © 2019-2019 liuyongqing<lyqdy1@163.com>
* v1.0.1 2019.05.04
*/
#ifndef __YMODBUS_YMBSLAVE_H__
#define __YMODBUS_YMBSLAVE_H__

#include "ymod/ymbdefs.h"
#include "ymod/ymbplayer.h"

#include <string>
#include <memory>

namespace YModbus {

class Slave final
{
public:
	Slave() = delete;
	Slave(const Slave&) = default;
	Slave(Slave&&) = default;
	Slave& operator=(Slave&&) = default;

	//prot: YMB_RTU/YMB_ASCII
	//nthr: thread count, = 0: need call Run
	Slave(const std::string& port,
		uint32_t baudrate,
		char parity, 
		uint8_t stopbits,
		eProtocol prot,
		eThreadMode thrm);

	//prot:
	//YMB_TCP/YMB_UDP
	//YMB_TCP_RTU/YMB_TCP_ASCII
	//YMB_UDP_RTU/YMB_UDP_ASCII
	//nthr: thread count, = 0: need call Run
	Slave(uint16_t port, eProtocol prot, eThreadMode thrm);

	~Slave();

	//By default, any id will receipt(id=0)
	void SetSlaveId(uint8_t id);
	uint8_t GetSlaveId(void) const;

	void SetPlayer(std::shared_ptr<IPlayer> player);
	std::shared_ptr<IPlayer> GetPlayer(void) const;

	bool Startup(void);
	void Shutdown(void);
	
	//for nthr = 0 to loop
	//to: timeout: ms
	//return: < 0: errorcode, = 0: no error
	int Run(long to);
	int GetLastError(void) const;

private:
	struct Impl;
	std::shared_ptr<Impl> impl_;
};

} //namespace YModbus

#endif // ! __YMODBUS_YMBSLAVE_H__

