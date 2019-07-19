/**
* ymodbus
* Copyright © 2019-2019 liuyongqing<lyqdy1@163.com>
* v1.0.1 2019.05.04
*/
#ifndef __YMODBUS_YMBSTORE_H__
#define __YMODBUS_YMBSTORE_H__

#include <cstdint>

namespace YModbus {

//数据为大端字节序
class IStore
{
public:
	//更新缓存的寄存器数据，同时更新生命值为最大, num:寄存器数目 val:网络序
	virtual void Set(uint8_t sid, uint16_t reg, const uint8_t *val, uint16_t num) = 0;

	//获取缓存的寄存器数据，如不存在或失效则返回失败
	virtual bool Get(uint8_t sid, uint16_t reg, uint8_t *val, uint16_t num) const = 0;

	//保存缓存的寄存器数据，同时更新生命值为永久
	virtual void Save(uint8_t sid, uint16_t reg, const uint8_t *val, uint16_t num) = 0;

	//强行加载，无论是否存在对应的设备，用于加载配置
	virtual void Load(uint8_t sid, uint16_t reg, const uint8_t *val, uint16_t num) = 0;

	virtual ~IStore() {}
};

} //namespace YModbus

#endif //__YMODBUS_YMBSTORE_H__
