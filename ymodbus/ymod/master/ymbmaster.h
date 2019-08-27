/**
* ymodbus
* Copyright © 2019-2019 liuyongqing<lyqdy1@163.com>
* v1.0.1 2019.05.04
*/
#ifndef __YMODBUS_YMBMASTER_H__
#define __YMODBUS_YMBMASTER_H__

#include "ymod/ymbdefs.h"
#include "ymod/ymbstore.h"
#include "ymod/ymbmonitor.h"
#include "ymod/ymbplayer.h"
#include "ymod/ymbutils.h"

#include <string>
#include <vector>
#include <memory>

namespace YModbus {

class Master : public IPlayer
{
public:
	Master(const std::string &ip, uint16_t port,
		eProtocol type, eThreadMode thrm);
	Master(const std::string &com, uint32_t baudrate,
		char parity, uint8_t stopbits, eProtocol type, eThreadMode thrm);

	Master() = delete;
	Master(const Master&) = default;
	Master &operator = (const Master &t) = default;
	Master(Master &&t) = default;
	Master &operator = (Master &&t) = default;

	~Master(void);

	void SetMonitor(std::shared_ptr<IMonitor> monitor);
	std::shared_ptr<IMonitor> GetMonitor(void) const;

	void SetStore(std::shared_ptr<IStore> store);
	std::shared_ptr<IStore> GetStore(void) const;

	//reties: retry count
	void SetRetries(uint32_t retries);
	uint32_t GetRetries(void) const;

	//rto: ms
	void SetReadTimeout(long rdto);
	long GetReadTimeout(void) const;

	//错误信息，线程相关，每个线程独立
	int GetLastError(void) const;
	std::string GetErrorString(int err) const;

	//检查连接状态，如未连接尝试连接一次
	bool CheckConnect(void);

	//异步抓取操作，不需返回读取的数据，也不需等待执行完成
	//返回的数据通过过SetStore的对象处理
	void PullCoils(uint8_t sid, uint16_t reg, uint16_t num);
	void PullDiscreteInputs(uint8_t sid, uint16_t reg, uint16_t num);
	void PullHoldingRegisters(uint8_t sid, uint16_t reg, uint16_t num);
	void PullInputRegisters(uint8_t sid, uint16_t reg, uint16_t num);

	//IPlayer================================================================
	//return: >= 0, bytes of data to return;
	//return: < 0, errorcode of exception
	//buf: data value, net order
	virtual int ReadCoils(uint8_t sid,
		uint16_t reg, uint16_t num, uint8_t *buf, size_t bufsiz);
	virtual int ReadDiscreteInputs(uint8_t sid,
		uint16_t reg, uint16_t num, uint8_t *buf, size_t bufsiz);
	virtual int ReadInputRegisters(uint8_t sid,
		uint16_t reg, uint16_t num, uint8_t *buf, size_t bufsiz);
	virtual int ReadHoldingRegisters(uint8_t sid,
		uint16_t reg, uint16_t num, uint8_t *buf, size_t bufsiz);

	//return: = 0, OK; 
	//return: < 0, errorcode of exception
	//values: data value, net order
	virtual int WriteSingleCoil(uint8_t sid, uint16_t reg, bool onoff);
	virtual int WriteCoils(uint8_t sid,
		uint16_t reg, uint16_t num, const uint8_t *bits, uint8_t wbytes);
	virtual int WriteSingleRegister(uint8_t sid, 
		uint16_t reg, uint16_t value);
	virtual int WriteRegisters(uint8_t sid,
		uint16_t reg, uint16_t num, const uint8_t *values, uint8_t wbytes);
	virtual int MaskWriteRegisters(uint8_t sid,
		uint16_t reg, uint16_t andmask, uint16_t ormask);

	//return: >= 0, bytes of data to return
	//return: < 0,  errorcode of exception
	//values/buf: data value, net order
	virtual int WriteReadRegisters(uint8_t sid,
		uint16_t wreg, uint16_t wnum, const uint8_t *values, uint8_t wbytes,
		uint16_t rreg, uint16_t rnum, uint8_t *buf, size_t bufsiz);

	//return: >= 0, OK
	//return: < 0,  errorcode of exception
	virtual int ReportSlaveId(uint8_t maxsid, uint8_t *buf, size_t bufsiz);

private:
	struct Impl;
	std::shared_ptr<Impl> impl_;
};

} //namespace YModbus

#endif //__YMODBUS_YMBMASTER_H__


