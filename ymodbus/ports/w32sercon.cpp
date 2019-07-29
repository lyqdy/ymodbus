/**
* ymodbus
* Copyright © 2019-2019 liuyongqing<lyqdy1@163.com>
* v1.0.1 2019.05.04
*/
#include "ymod/master/yserconnect.h"
#include "ymod/ymbdefs.h"
#include "ymblog.h"

#include <Windows.h>

namespace YModbus {

struct SerConnect::Impl
{
	Impl(const std::string &com,
		uint32_t baudrate, uint8_t databits, char parity, uint8_t stopbits)
		: file_(INVALID_HANDLE_VALUE)
	{
		/* Open the serial device. */
		file_ = CreateFileA(com.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL,
			OPEN_EXISTING, 0, NULL);

		if (file_ == INVALID_HANDLE_VALUE) {
			LOG(ERROR) << "Open serial failed! "
				<< com << ", "
				<< baudrate << "bps, "
				<< static_cast<int>(databits) << ", "
				<< parity << ", "
				<< stopbits / 10.0 << std::endl;
			return;
		}

		DCB dcb = {0};
		dcb.DCBlength = sizeof(dcb);

		dcb.BaudRate = baudrate;
		dcb.ByteSize = databits;

		if (stopbits == kSerStopbits1)
			dcb.StopBits = ONESTOPBIT;
		else if (stopbits == kSerStopbits15)
			dcb.StopBits = ONE5STOPBITS;
		else
			dcb.StopBits = TWOSTOPBITS;

		if (parity == kSerParityNone) {
			dcb.Parity = NOPARITY;
			dcb.fParity = FALSE;
		}
		else if (parity == kSerParityEven) {
			dcb.Parity = EVENPARITY;
			dcb.fParity = TRUE;
		}
		else {
			dcb.Parity = ODDPARITY;
			dcb.fParity = TRUE;
		}

		dcb.fTXContinueOnXoff = TRUE;
		dcb.fOutX = FALSE;
		dcb.fInX = FALSE;
		dcb.fBinary = TRUE;
		dcb.fAbortOnError = FALSE;

		if (!SetCommState(file_, &dcb)) {
			CloseHandle(file_);
			file_ = INVALID_HANDLE_VALUE;
			LOG(ERROR) << "SetCommState failed! " << com << std::endl;
		}
	}
	
	bool SetReadTimeout(long to)
	{
		COMMTIMEOUTS    cto;
		cto.ReadIntervalTimeout = 4; //modbus < 3.5ms
		cto.ReadTotalTimeoutConstant = static_cast<DWORD>(to);
		cto.ReadTotalTimeoutMultiplier = 0;
		cto.WriteTotalTimeoutConstant = 0;
		cto.WriteTotalTimeoutMultiplier = 0;

		return static_cast<bool>(SetCommTimeouts(file_, &cto));
	}

	~Impl()
	{
		if (file_ != INVALID_HANDLE_VALUE) {
			CloseHandle(file_);
			file_ = INVALID_HANDLE_VALUE;
		}
	}

	HANDLE file_;
};

SerConnect::SerConnect(const std::string &com,
	uint32_t baudrate, uint8_t databits, char parity, uint8_t stopbits)
	: impl_(std::make_unique<Impl>(com, baudrate, databits, parity, stopbits))
{

}

SerConnect::~SerConnect()
{

}

void SerConnect::SetTimeout(long to)
{
	impl_->SetReadTimeout(to);
}

bool SerConnect::Validate(void)
{
	return impl_->file_ != INVALID_HANDLE_VALUE;
}

void SerConnect::Purge(void)
{
	if (impl_->file_ != INVALID_HANDLE_VALUE) {
		PurgeComm(impl_->file_, PURGE_RXCLEAR | PURGE_TXCLEAR);
	}
}

bool SerConnect::Send(uint8_t *buf, size_t len)
{
	if (impl_->file_ != INVALID_HANDLE_VALUE) {
		char *pbuf = reinterpret_cast<char *>(buf);
		DWORD dwWritten;
		do {
			if (WriteFile(impl_->file_, pbuf, len, &dwWritten, NULL)) {
				len -= static_cast<size_t>(dwWritten);
				pbuf += static_cast<size_t>(dwWritten);
			}
			else {
				return false;
			}
		} while (len != 0);
	}

	return len == 0;
}

int SerConnect::Recv(uint8_t *buf, size_t len)
{
	if (impl_->file_ != INVALID_HANDLE_VALUE) {
		DWORD dwRead;
		if (ReadFile(impl_->file_, buf, len, &dwRead, NULL)) 
			return static_cast<int>(dwRead);
	}
	
	return -ENODEV;
}

} //namespace YModbus
