/**
* ymodbus
* Copyright © 2019-2019 liuyongqing<lyqdy1@163.com>
* v1.0.1 2019.05.04
*/
#include "ymod/slave/yserlistener.h"
#include "ymod/ymbdefs.h"
#include "ymblog.h"
#include "ymbopts.h"

#include <Windows.h>
#include <algorithm>
#include <iomanip>

namespace YModbus {

struct SerListener::Impl : public ISession
{
	Impl(const std::string &port) 
		: port_(port), recvlen_(0) {}
	virtual std::string PeerName(void);
	virtual int Write(uint8_t *msg, size_t msglen);
	virtual int Read(uint8_t *buf, size_t bufsiz);
	virtual size_t Peek(uint8_t **buf);

	virtual void Purge(void);
	virtual void Discard(size_t nbytes);

	bool Recv(void);

	std::string port_;
	HANDLE file_;
	char recvbuf_[kMaxMsgLen];
	size_t recvlen_;
};

std::string SerListener::Impl::PeerName()
{
	return std::string("ser:") + port_;
}

int SerListener::Impl::Write(uint8_t *msg, size_t msglen)
{
	if (file_ != INVALID_HANDLE_VALUE) {
		DWORD dwWritten;
		size_t len = msglen;
		uint8_t *pbuf = msg;
		do {
			if (WriteFile(file_, pbuf, len, &dwWritten, NULL)) {
				len -= dwWritten;
				pbuf += dwWritten;
				FlushFileBuffers(file_);
			}
			else {
				return -EFAULT; //error
			}
		} while (len != 0);

		YMB_HEXDUMP(msg, msglen,
			"%s write data, len = %u: ", port_.c_str(), dwWritten);

		return msglen;
	}

	return -EDEV;
}

int SerListener::Impl::Read(uint8_t *buf, size_t bufsiz)
{
	if (recvlen_ < bufsiz)
		bufsiz = recvlen_;

	memcpy(buf, recvbuf_, bufsiz);
	recvlen_ -= bufsiz;

	return static_cast<int>(bufsiz);
}

size_t SerListener::Impl::Peek(uint8_t **buf)
{
	*buf = reinterpret_cast<uint8_t*>(&recvbuf_[0]);
	return recvlen_;
}

void SerListener::Impl::Purge(void)
{
	recvlen_ = 0;

	if (file_ != INVALID_HANDLE_VALUE)
		::PurgeComm(file_, PURGE_RXCLEAR | PURGE_TXCLEAR);
}

void SerListener::Impl::Discard(size_t nbytes)
{
	if (recvlen_ > nbytes) {
		recvlen_ -= nbytes;
		memmove(recvbuf_, recvbuf_ + nbytes, recvlen_);
	}
	else {
		recvlen_ = 0;
	}
}

bool SerListener::Impl::Recv()
{
	if (file_ != INVALID_HANDLE_VALUE) {
		DWORD dwRead;

		if (recvlen_ == sizeof(recvbuf_))
			recvlen_ = 0;
		
		if (ReadFile(file_, recvbuf_ + recvlen_,
			sizeof(recvbuf_) - recvlen_, &dwRead, NULL)) {
			if (dwRead != 0) { //some bytes arrived
				recvlen_ += dwRead;
				YMB_HEXDUMP0(recvbuf_, recvlen_, "%s recvbuf, len = %u",
					port_.c_str(), recvlen_);
				return true;
			}
		}
	}

	return false;
}

SerListener::SerListener(const std::string& port,
	uint32_t baudrate,
	uint8_t databits,
	char parity,
	uint8_t stopbits)
	: impl_(std::make_shared<Impl>(port))
{
	DCB dcb;
	
	impl_->file_ = CreateFileA(
		port.c_str(),
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);

	if (impl_->file_ == INVALID_HANDLE_VALUE) {
		LOG(ERROR) << "Open serial failed! "
			<< port << ", "
			<< baudrate << "bps, "
			<< static_cast<int>(databits) << ", "
			<< parity << ", "
			<< std::setprecision(1) << stopbits / 10.0 << std::endl;
		return;
	}

	dcb.DCBlength = sizeof(DCB);
	if (!GetCommState(impl_->file_, &dcb)) {
		CloseHandle(impl_->file_);
		impl_->file_ = INVALID_HANDLE_VALUE;
		LOG(ERROR) << "GetCommState failed! "
			<< port << std::endl;
		return;
	}

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

	if (!SetCommState(impl_->file_, &dcb)) {
		CloseHandle(impl_->file_);
		impl_->file_ = INVALID_HANDLE_VALUE;
		LOG(ERROR) << "SetCommState failed! "
			<< port << std::endl;
		return;
	}

	YMB_DEBUG("Open serial success! %s, %u, %u, %c, %3.1f\n",
		port.c_str(), baudrate, databits, parity, stopbits / 10.0);
}

std::string SerListener::GetName(void)
{
	return impl_->PeerName();
}

void SerListener::SetTimeout(long to)
{
	if (impl_->file_ != INVALID_HANDLE_VALUE) {
		COMMTIMEOUTS cto = {0};

		cto.ReadIntervalTimeout = 5;
		cto.ReadTotalTimeoutConstant = 5;
		cto.WriteTotalTimeoutMultiplier = 0;
		cto.WriteTotalTimeoutConstant = 1000;

		SetCommTimeouts(impl_->file_, &cto);
	}
}

bool SerListener::Listen(void)
{
	return impl_->file_ != INVALID_HANDLE_VALUE;
}

int SerListener::Accept(std::vector<SessionPtr> &ses)
{
	ses.clear();

	if (impl_->Recv()) {
		//data received, impl_ is the session
		ses.push_back(impl_); 
	}

	return EOK; //no error
}

SerListener::~SerListener()
{
	if (impl_->file_ != INVALID_HANDLE_VALUE) {
		CloseHandle(impl_->file_);
		impl_->file_ = INVALID_HANDLE_VALUE;
	}
}

} //namespace YModbus

