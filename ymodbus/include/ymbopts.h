/**
* ymodbus
* Copyright © 2019-2019 liuyongqing<lyqdy1@163.com>
* v1.0.1 2019.05.04
*/
#ifndef __YMODBUS_YMBOPTS_H__
#define __YMODBUS_YMBOPTS_H__

#include <cstdint>
#include <cstddef>

namespace YModbus {

const size_t kMaxTcpSessionNum = 256;
const uint16_t kMaxSerailPort = 2;
const size_t kMaxMsgLen = (512 + 7);

} //namespace YModbus

#endif // ! __YMODBUS_YMBOPTS_H__

