/**
* ymodbus
* Copyright © 2019-2019 liuyongqing<lyqdy1@163.com>
* v1.0.1 2019.05.04
*/
// test_yslave.cpp 
//
#include "ymod/slave/ymbslave.h"
#include "ymod/slave/yslave.h"

namespace YModbus {

class Player : public IPlayer
{
public:
	//return: >= 0, bytes of data to return;
	//return: < 0, errorcode of exception
	virtual int ReadCoils(uint8_t sid,
		uint16_t reg, uint16_t num, uint8_t *buf, size_t bufsiz)
	{
		*buf++ = 0x01;
		*buf++ = 0x02;
		return num / 8 + ((num % 8) != 0 ? 1 : 0);
	}
	virtual int ReadDiscreteInputs(uint8_t sid,
		uint16_t reg, uint16_t num, uint8_t *buf, size_t bufsiz)
	{
		*buf++ = 0x01;
		*buf++ = 0x02;
		return num / 8 + ((num % 8) != 0 ? 1 : 0);
	}
	virtual int ReadInputRegisters(uint8_t sid,
		uint16_t reg, uint16_t num, uint8_t *buf, size_t bufsiz)
	{
		*buf++ = 0x01;
		*buf++ = 0x02;
		return num * 2;
	}
	virtual int ReadHoldingRegisters(uint8_t sid,
		uint16_t reg, uint16_t num, uint8_t *buf, size_t bufsiz)
	{
		*buf++ = 0x01;
		*buf++ = 0x02;
		*buf++ = 0x03;
		*buf++ = 0x04;
		*buf++ = 0x05;
		*buf++ = 0x06;
		*buf++ = 0x07;
		*buf++ = 0x08;
		return num * 2;
	}

	//return: >= 0, OK; 
	//return: < 0, errorcode of exception
	virtual int WriteSingleCoil(uint8_t sid,
		uint16_t reg, bool onoff)
	{
		return 0;
	}
	virtual int WriteCoils(uint8_t sid,
		uint16_t reg, uint16_t num, const uint8_t *bits, uint8_t wbytes)
	{
		return 0;
	}
	virtual int WriteSingleRegister(uint8_t sid,
		uint16_t reg, uint16_t value)
	{
		return 0;
	}
	virtual int WriteRegisters(uint8_t sid,
		uint16_t reg, uint16_t num, const uint8_t *values, uint8_t wbytes)
	{
		return 0;
	}
	virtual int MaskWriteRegisters(uint8_t sid,
		uint16_t reg, uint16_t andmask, uint16_t ormask)
	{
		return 0;
	}

	//return: >= 0, bytes of data to return
	//return: < 0,  errorcode of exception
	virtual int WriteReadRegisters(uint8_t sid,
		uint16_t wreg, uint16_t wnum, const uint8_t *values, uint8_t wbytes,
		uint16_t rreg, uint16_t rnum, uint8_t *buf, size_t bufsiz)
	{
		*buf++ = 0x01;
		*buf++ = 0x02;
		return rnum * 2;
	}

	//return: >= 0, OK
	//return: < 0,  errorcode of exception
	virtual int ReportSlaveId(uint8_t maxsid, uint8_t *buf, size_t bufsiz)
	{
		return -1;
	}
};

} //namespace YModbus

using namespace YModbus;

int main()
{
	Slave slave1(5502, TCP, TASK);
	Slave slave2(5503, TCPRTU, TASK);
	Slave slave3(5504, TCPASCII, TASK);

#ifdef WIN32
	//Slave slave4("COM3", 256000, kSerParityNone, kSerStopbits1, RTU, TASK);
#else
	Slave slave4("/dev/ttyS9", 115200, kSerParityNone, kSerStopbits1, YMB_PROT_RTU, YMB_THR_POLL);
#endif

	TSlave<SNet, TcpListener, Player> slave5(5505, TASK);
	TSlave<SRtu, TcpListener, Player> slave6(5506, TASK);
	TSlave<SAscii, TcpListener, Player> slave7(5507, TASK);

	Slave slave8(5508, UDP, TASK);

	slave1.SetPlayer(std::make_shared<YModbus::Player>());
	slave2.SetPlayer(std::make_shared<YModbus::Player>());
	slave3.SetPlayer(std::make_shared<YModbus::Player>());
	//slave4.SetPlayer(std::make_shared<YModbus::Player>());
	slave5.SetPlayer(std::make_shared<YModbus::Player>());
	slave6.SetPlayer(std::make_shared<YModbus::Player>());
	slave7.SetPlayer(std::make_shared<YModbus::Player>());
	slave8.SetPlayer(std::make_shared<YModbus::Player>());

	slave1.Startup();
	slave2.Startup();
	slave3.Startup();
	//slave4.Startup();
	slave5.Startup();
	slave6.Startup();
	slave7.Startup();
	slave8.Startup();

	Task::LetUsGo();

	while (1);

    return 0;
}

