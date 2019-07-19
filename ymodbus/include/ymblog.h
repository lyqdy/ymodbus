/**
* ymodbus
* Copyright © 2019-2019 liuyongqing<lyqdy1@163.com>
* v1.0.1 2019.05.04
*/
#ifndef __YMODBUS_YMBLOG_H__
#define __YMODBUS_YMBLOG_H__

#include <iostream>
#include <stdio.h>
#include <string>
#include <fstream>

#ifndef WIN32
#	include <sys/types.h>
#	include <sys/stat.h>
#	define GLOG_NO_ABBREVIATED_SEVERITIES
#	include "glog/logging.h"
#else
#	include <direct.h>
#	define LOG(x) std::cout << #x << ":"
#endif

#ifndef ASSERT
#	include <assert.h>
#	define ASSERT(x) assert(x)
#endif

#define YMB_ASSERT(x) do { (void)(x); ASSERT(x); } while (0)
#define YMB_VERIFY(x) do { if (!(x)) ASSERT(0&&(#x)); } while (0)

#define YMB_ASSERT_MSG(x, fmt, ...) do {					\
	(void)(x);												\
	fprintf(stderr, fmt, ##__VA_ARGS__);					\
	ASSERT(x);												\
} while (0)

#ifndef NDEBUG

#define YMB_DEBUG(fmt, ...) fprintf(stdout, fmt, ##__VA_ARGS__)
#define YMB_ERROR(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)

#define YMB_HEXDUMP(_xbuf, _xlen, fmt, ...)						\
	do {														\
		uint8_t *__xbuf = (uint8_t *)(_xbuf);					\
		fprintf(stdout, fmt, ##__VA_ARGS__);					\
		for (size_t _i = 0; _i < (size_t)(_xlen); ++_i) {		\
			fprintf(stdout, "%02X ", __xbuf[_i]);				\
			if (((_i + 1) % 128) == 0)							\
				fprintf(stdout, "\n");							\
		}														\
		fprintf(stdout, "\n");									\
	} while (0)

#else

#define YMB_DEBUG(fmt, ...)
#define YMB_ERROR(fmt, ...)

#define YMB_HEXDUMP(_xbuf, _xlen, fmt, ...)

#endif //NDEBUG

#define YMB_DEBUG0(fmt, ...)
#define YMB_ERROR0(fmt, ...)

#define YMB_HEXDUMP0(_xbuf, _xlen, fmt, ...) (void)(_xbuf)

#endif // __YMODBUS_YMBLOG_H__
