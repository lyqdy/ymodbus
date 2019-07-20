/**
* ymodbus
* Copyright © 2019-2019 liuyongqing<lyqdy1@163.com>
* v1.0.1 2019.05.04
*/
#include "ymblog.h"

#include "ymod/ymbutils.h"
#include "ymod/ymbtask.h"

#include "ymod/master/ymbmaster.h"
#include "ymod/master/ymaster.h"

#include <iomanip>

void LOG_Init(char *prog) {}
void LOG_Fini(void) {}

using namespace YModbus;

template<typename T>
class A
{
public:
	void f(uint8_t sid, uint16_t startreg, T val)
	{
		YMB_DEBUG("AsyncRead val = %x(%u)\n", val, val);
	}
};

using namespace std::placeholders;

int main(int argc, char *argv[])
{
	LOG_Init(argv[0]);
	LOG(INFO) << "test master enter" << std::endl;

	Master tcpmaster("127.0.0.1", 5502, TCP, TASK);
	TMaster<MRtu, TcpConnect> rtumaster("127.0.0.1", 5506, TASK);
	TMaster<MRtu, TcpConnect> rtu2master("127.0.0.1", 5503, TASK);
	TMaster<MNet, UdpConnect> udpmaster("127.0.0.1", 5508, TASK);

	rtumaster.SetReadTimeout(1000);
	tcpmaster.SetReadTimeout(500);
	udpmaster.SetReadTimeout(500);

	Task::LetUsGo();

	uint8_t buf[BUFSIZ];
	int len;

	for (int i = 0; i < 500; i++)
	{
		std::shared_ptr<A<uint16_t>> a = std::make_shared<A<uint16_t>>();
		udpmaster.AsyncRead<uint16_t>(1, 1, std::bind(&A<uint16_t>::f, a, _1, _2, _3));

		std::shared_ptr<A<uint32_t>> b = std::make_shared<A<uint32_t>>();
		udpmaster.AsyncRead<uint32_t>(1, 1, std::bind(&A<uint32_t>::f, b, _1, _2, _3));
	}

	udpmaster.WaitAsynReader();


	udpmaster.SetByteOrder(BOR_1234);
	{
		uint16_t val = 0;
		udpmaster.ReadValue(1, 1, val);
		YMB_DEBUG("ReadValue BOR_1234= %x(%u)\n", val, val);
	}

	{
		uint32_t val = 0;
		udpmaster.ReadValue(1, 1, val);
		YMB_DEBUG("ReadValue BOR_1234= %x(%lu)\n", val, val);
	}

	{
		uint64_t val = 0;
		udpmaster.ReadValue(1, 1, val);
		YMB_DEBUG("ReadValue BOR_1234= %llx(%llu)\n", val, val);
	}

	rtumaster.SetByteOrder(BOR_1234);
	{
		uint16_t val = 0;
		rtumaster.ReadValue(1, 1, val);
		YMB_DEBUG("ReadValue BOR_1234= %x(%u)\n", val, val);
	}

	{
		uint32_t val = 0;
		rtumaster.ReadValue(1, 1, val);
		YMB_DEBUG("ReadValue BOR_1234= %x(%lu)\n", val, val);
	}

	{
		uint64_t val = 0;
		rtumaster.ReadValue(1, 1, val);
		YMB_DEBUG("ReadValue BOR_1234= %llx(%llu)\n", val, val);
	}

	rtumaster.SetByteOrder(BOR_3412);
	{
		uint16_t val = 0;
		rtumaster.ReadValue(1, 1, val);
		YMB_DEBUG("ReadValue BOR_3412= %x(%u)\n", val, val);
	}

	{
		uint32_t val = 0;
		rtumaster.ReadValue(1, 1, val);
		YMB_DEBUG("ReadValue BOR_3412= %x(%lu)\n", val, val);
	}

	{
		uint64_t val = 0;
		rtumaster.ReadValue(1, 1, val);
		YMB_DEBUG("ReadValue BOR_3412= %llx(%llu)\n", val, val);
	}

	rtumaster.SetByteOrder(BOR_4321);
	{
		uint16_t val = 0;
		rtumaster.ReadValue(1, 1, val);
		YMB_DEBUG("ReadValue BOR_4321= %x(%u)\n", val, val);
	}

	{
		uint32_t val = 0;
		rtumaster.ReadValue(1, 1, val);
		YMB_DEBUG("ReadValue BOR_4321= %x(%lu)\n", val, val);
	}

	{
		uint64_t val = 0;
		rtumaster.ReadValue(1, 1, val);
		YMB_DEBUG("ReadValue BOR_4321= %llx(%llu)\n", val, val);
	}

	rtumaster.SetByteOrder(BOR_2143);
	{
		uint16_t val = 0;
		rtumaster.ReadValue(1, 1, val);
		YMB_DEBUG("ReadValue BOR_2143= %x(%u)\n", val, val);
	}

	{
		uint32_t val = 0;
		rtumaster.ReadValue(1, 1, val);
		YMB_DEBUG("ReadValue BOR_2143= %x(%lu)\n", val, val);
	}

	{
		uint64_t val = 0;
		rtumaster.ReadValue(1, 1, val);
		YMB_DEBUG("ReadValue BOR_2143= %llx(%llu)\n", val, val);
	}

	exit(0);

	len = tcpmaster.ReadHoldingRegisters(1, 1, 1, buf, sizeof(buf));
	if (len > 0) {
		YMB_HEXDUMP(buf, len, "tcpReadHoldingRegisters: ");
	}
	YMB_ASSERT(len == 2);

	uint32_t rcount = 0;
	uint32_t ecount = 0;

	do {
		rcount++;
		len = tcpmaster.ReadHoldingRegisters(1, 1, 10, buf, sizeof(buf));
		if (len > 0) {
			YMB_HEXDUMP0(buf, len, "tcpReadHoldingRegisters: ");
		}
		else {
			ecount++;
		}
		YMB_DEBUG("REQ:%u, ERR:%u\n", rcount, ecount);
	}  while (rcount < 10000);

	len = tcpmaster.ReadHoldingRegisters(1, 1, 100, buf, sizeof(buf));
	if (len > 0) {
		YMB_HEXDUMP(buf, len, "tcpReadHoldingRegisters: ");
	}
	YMB_ASSERT(len == 200);
		

	len = rtumaster.ReadHoldingRegisters(1, 1, 1, buf, sizeof(buf));
	if (len > 0) {
		YMB_HEXDUMP(buf, len, "rtuReadHoldingRegisters: ");
	}
	YMB_ASSERT(len == 2);

	len = rtumaster.ReadHoldingRegisters(1, 1, 10, buf, sizeof(buf));
	if (len > 0) {
		YMB_HEXDUMP(buf, len, "rtuReadHoldingRegisters: ");
	}
	YMB_ASSERT(len == 20);

	rcount = 0;
	ecount = 0;
	do {
		rcount++;
		len = rtumaster.ReadHoldingRegisters(1, 1, 10, buf, sizeof(buf));
		if (len > 0) {
			YMB_HEXDUMP0(buf, len, "tcpReadHoldingRegisters: ");
		}
		else {
			ecount++;
		}
		YMB_DEBUG("REQ:%u, ERR:%u\n", rcount, ecount);
	} while (rcount < 10000);

	len = rtumaster.ReadHoldingRegisters(1, 1, 100, buf, sizeof(buf));
	if (len > 0) {
		YMB_HEXDUMP(buf, len, "rtuReadHoldingRegisters: ");
	}
	YMB_ASSERT(len == 200);

	len = rtu2master.ReadHoldingRegisters(1, 1, 1, buf, sizeof(buf));
	if (len > 0) {
		YMB_HEXDUMP(buf, len, "rtuReadHoldingRegisters: ");
	}
	YMB_ASSERT(len == 2);

	rcount = 0;
	ecount = 0;
	do {
		rcount++;
		len = rtu2master.ReadHoldingRegisters(1, 1, 10, buf, sizeof(buf));
		if (len > 0) {
			YMB_HEXDUMP0(buf, len, "tcpReadHoldingRegisters: ");
		}
		else {
			ecount++;
		}
		YMB_DEBUG("REQ:%u, ERR:%u\n", rcount, ecount);
	} while (rcount < 10000);

	Task::LetUsDone();

	LOG(INFO) << "test master quit" << std::endl;
	LOG_Fini();

	return 0;
}

