/**
 * ymodbus
 * Copyright © 2019-2019 liuyongqing<lyqdy1@163.com>
 * v1.0.1 2019.05.04
 */
#ifndef __YMODBUS_YMBUTILS_H__
#define __YMODBUS_YMBUTILS_H__

#include "ymod/ymbdefs.h"
#include "ymblog.h"

#include <algorithm>

namespace YModbus {

inline bool IsLittleEndian(void)
{
	static unsigned long x = 0x12345678;
	return *reinterpret_cast<unsigned char*>(&x) == 0x78;
}

template<typename T>
inline void YExchangeByteOrder(T &val)
{
	char *p = reinterpret_cast<char*>(&val);
	std::reverse(p, p + sizeof(val));
}

//template<typename T>
//inline void YNetToHost(T &val, eByteOrder bor)
//T may be integral or float/double type, such as int, uint64_t...
//If you have some custom structs defined by yourself, 
//	you must override the function template for your own struct.
//Example:
//	struct A {
//		uint16_t len;
//		char name[MAX_NAME_LEN];
//  };
//  template<>
//  inline void YNetToHost<A>(A &val, eByteOrder bor)
//  {
//		//Nothing to do for raw data or your custom codes.
//		YNetToHost(val.len, bor);
//  }
//  A a;
//  if (master.ReadValue(sid, startreg, a)) {
//		....
//	}
//
template<typename T>
inline void YNetToHost(T &val, eByteOrder bor)
{
	if (IsLittleEndian()) { //host is little endian
		switch (bor) {
		case BOR_1234: //Big-endian
			//0x01020304=>{0x01,0x02,0x03,0x04}=>{0x04,0x03,0x02,0x01}
			YExchangeByteOrder(val);
			break;
		case BOR_3412: { //Little-endian byte swap
			//0x01020304=>{0x03,0x04,0x01,0x02}=>{0x04,0x03,0x02,0x01}
			uint16_t *p = reinterpret_cast<uint16_t*>(&val);
			std::for_each(p, p + sizeof(val) / 2, [](uint16_t &v) {
				YExchangeByteOrder(v);
			});
			break; }
		case BOR_4321:	//Little-endian
			//0x01020304=>{0x04,0x03,0x02,0x01}=>{0x04,0x03,0x02,0x01}
			//Nothing to do
			break;
		case BOR_2143: { // Big-endian byte swap
			//0x01020304=>{0x02,0x01,0x04,0x03}=>{0x04,0x03,0x02,0x01}
			uint16_t *p = reinterpret_cast<uint16_t*>(&val);
			std::reverse(p, p + sizeof(val) / 2);
			break; }
		default:
			YMB_ASSERT(false);
			break;
		} //switch (bor)
	} //host is little-endian
	else { //host is big-endian
		switch (bor) {
		case BOR_1234: //Big-endian
			//0x01020304=>{0x01,0x02,0x03,0x04}=>{0x01,0x02,0x03,0x04}
			//Nothing to do
			break;
		case BOR_3412: { //Little-endian byte swap
			//0x01020304=>{0x03,0x04,0x01,0x02}=>{0x01,0x02,0x03,0x04}
			uint16_t *p = reinterpret_cast<uint16_t*>(&val);
			std::reverse(p, p + sizeof(val) / 2);
			break; }
		case BOR_4321:	//Little endian
			//0x01020304=>{0x04,0x03,0x02,0x01}=>{0x01,0x02,0x03,0x04}
			YExchangeByteOrder(val);
			break;
		case BOR_2143: { // big-endian byte swap
			//0x01020304=>{0x02,0x01,0x04,0x03}=>{0x01,0x02,0x03,0x04}
			uint16_t *p = reinterpret_cast<uint16_t*>(&val);
			std::for_each(p, p + sizeof(val) / 2, [](uint16_t &v) {
				YExchangeByteOrder(v);
			});
			break; }
		default:
			YMB_ASSERT(false);
			break;
		} //switch (bor)
	} //host is big-endian
}

#define YHostToNet YNetToHost

} //namesapce YModbus

#if defined(__GNUC__) && __GNUC__ < 5 //for std::make_unique

#include <type_traits>
#include <memory>

namespace std {

template<class T, class... Args> inline
typename enable_if<!is_array<T>::value, unique_ptr<T>>::type
make_unique(Args&&... args) {
	return unique_ptr<T>(new T(std::forward<Args>(args)...));
}

template<class T> inline
typename enable_if<is_array<T>::value && extent<T>::value == 0, unique_ptr<T>>::type
make_unique(size_t size) {
	typedef typename remove_extent<T>::type U;
	return unique_ptr<T>(new U[size]());
}

template<class T, class... Args>
typename enable_if<extent<T>::value != 0, void>::type
make_unique(Args&&...) = delete;

} //namespace std

#else

#	include <memory>

#endif


namespace YModbus {

//for function argue type
template<int index, typename FuntionType>
struct ArgTypeAt;

template<typename ResultType, typename FirstArgType, typename... ArgsType>
struct ArgTypeAt<0, ResultType(FirstArgType, ArgsType...)>
{
	using type = FirstArgType;
};

template<int index, typename ResultType, typename FirstArgType, typename... ArgsType>
struct ArgTypeAt<index, ResultType(FirstArgType, ArgsType...)>
{
	using type = typename ArgTypeAt<index - 1, ResultType(ArgsType...)>::type;
};

} //namespace YModbus

#endif //__YMODBUS_YMBUTILS_H__

