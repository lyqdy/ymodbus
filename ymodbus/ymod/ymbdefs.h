/**
* ymodbus
* Copyright © 2019-2019 liuyongqing<lyqdy1@163.com>
* v1.0.1 2019.05.04
*/
#ifndef __YMODBUS_YMBDEFS_H__
#define __YMODBUS_YMBDEFS_H__

#include <cstdint>
#include <cstddef>

#define kSerParityNone		'N'	/*!< No parity. */
#define kSerParityOdd		'O'	/*!< Odd parity. */
#define kSerParityEven		'E'	/*!< Even parity. */

#define kSerStopbits1		10 // NESTOPBIT
#define kSerStopbits15		15 // ONE5STOPBITS        
#define kSerStopbits2		20 // TWOSTOPBITS         

#define kMaxRegNum			125
#define kAnySlaveId			0
#define kBroadcastId		0

namespace YModbus {

typedef enum {
	RTU,
	ASCII,
	
	TCP,
	TCPRTU,
	TCPASCII,

	UDP,
	UDPRTU,
	UDPASCII,
} eProtocol;

#define PROTNAME(prot)					\
	((prot == RTU ? "RTU" :				\
	 (prot == ASCII ? "ASCII" :			\
	 (prot == TCP ? "TCP" :				\
	 (prot == TCPRTU ? "TCPRTU" :		\
	 (prot == TCPASCII ? "TCPASCII" :	\
	 (prot == UDP ? "UDP" :				\
	 (prot == UDPRTU ? "UDPRTU" :		\
	 (prot == UDPASCII ? "UDPASCII" : "ERROR")))))))))

typedef enum {
	POLL = 0,
	TASK = 1,
} eThreadMode;

typedef enum {
	SLAVE = 0,
	MASTER = 1,
} eMsgDirect;

typedef enum {
	EOK = 0,
	EFUN,
	EREG,
	EVAL,
	EDEV,
	EACK,
	EYBUSY,
} eModbusError;

typedef enum {
	BOR_1234, //0x01020304=>{ 0x01, 0x02, 0x03, 0x04 } big-endian
	BOR_3412, //0x01020304=>{ 0x03, 0x04, 0x01, 0x02 } little-endian byte swap
	BOR_4321, //0x01020304=>{ 0x04, 0x03, 0x02, 0x01 } little-endian
	BOR_2143, //0x01020304=>{ 0x02, 0x01, 0x04, 0x03 } big-endian byte swap
} eByteOrder;

typedef enum {
	SM_OriginalOctetStream,
	SM_XchangedByteStream
} eStringMode;

const uint8_t kFunReadCoils				= 0x01;
const uint8_t kFunReadDiscreteInputs	= 0x02;
const uint8_t kFunReadHoldingRegisters	= 0x03;
const uint8_t kFunReadInputRegisters	= 0x04;

const uint8_t kFunWriteSingleCoil		= 0x05;
const uint8_t kFunWriteSingleRegister	= 0x06;
const uint8_t kFunWriteMultiCoils		= 0x0F;
const uint8_t kFunWriteMultiRegisters	= 0x10;
const uint8_t kFunMaskWriteRegister		= 0x16;
const uint8_t kFunWriteAndReadRegisters = 0x17;

#define INVALID_REG 0xffff
#define INVALID_NUM 0xffff

} //namespace YModbus

#endif //__YMODBUS_YMBDEFS_H__

