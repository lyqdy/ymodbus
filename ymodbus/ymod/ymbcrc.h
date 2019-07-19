/**
* ymodbus
* Copyright © 2019-2019 liuyongqing<lyqdy1@163.com>
* v1.0.1 2019.05.04
*/
#ifndef __YMODBUS_YMBCRC_H__
#define __YMODBUS_YMBCRC_H__

#include <cstdint>

namespace YModbus {
	
uint16_t Crc16(uint8_t *msg, uint16_t len);

} //namespace YModbus

#endif // ! __YMODBUS_YMBCRC_H__

