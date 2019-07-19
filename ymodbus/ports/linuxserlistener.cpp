/**
* ymodbus
* Copyright © 2019-2019 liuyongqing<lyqdy1@163.com>
* v1.0.1 2019.05.04
*/
#include "ymod/slave/yserlistener.h"
#include "ymod/ymbdefs.h"
#include "ymblog.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/time.h>

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

	int fd_;
	timeval tv_ = { 0, 0 };
	char recvbuf_[kMaxMsgLen];
	size_t recvlen_;
};

std::string SerListener::Impl::PeerName()
{
	return std::string("ser:") + port_;
}

int SerListener::Impl::Write(uint8_t *msg, size_t msglen)
{
	if (fd_ != -1) {
		int  ret;
		size_t len = msglen;

		YMB_HEXDUMP0(msg, msglen, "Ser Write fd = %d", fd_);
		do {
			ret = write(fd_, msg, len);
			if (ret > 0) {
				YMB_HEXDUMP0(msg, ret, "Ser Send fd = %d", fd_);
				len -= ret;
				msg += ret;
			}
			else {
				return -1; //error
			}
		} while (len != 0);

		return msglen;
	}

	return -1;
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

	if (fd_ != -1)
		tcflush(fd_, TCIOFLUSH);
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
	if (recvlen_ == sizeof(recvbuf_))
		recvlen_ = 0;

	int len = read(fd_,
		&recvbuf_[recvlen_], sizeof(recvbuf_) - recvlen_);

	if (len > 0) {
		recvlen_ += len;
		YMB_HEXDUMP0(recvbuf_, recvlen_,
			"%s recvbuf, len = %u ", PeerName().c_str(), recvlen_);
		return true;
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
	impl_->fd_ = open(port.c_str(), O_RDWR | O_NOCTTY);
	if (impl_->fd_ < 0) {
		impl_->fd_ = -1;
		LOG(ERROR) << "Open serial failed! "
			<< port << ", "
			<< baudrate << "bps, "
			<< static_cast<int>(databits) << ", "
			<< parity << ", "
			<< std::setprecision(1) <<stopbits / 10.0 << std::endl;
		return;
	}

	struct termios  tio = { 0 };

	tio.c_iflag |= IGNBRK | INPCK;
	tio.c_cflag |= CREAD | CLOCAL;

	switch (parity)
	{
	case kSerParityNone:
		break;
	case kSerParityEven:
		tio.c_cflag |= PARENB;
		break;
	case kSerParityOdd:
		tio.c_cflag |= PARENB | PARODD;
		break;
	default:
		break;
	}

	switch (databits)
	{
	case 8:
		tio.c_cflag |= CS8;
		break;
	case 7:
		tio.c_cflag |= CS7;
		break;
	default:
		tio.c_cflag |= CS8;
		break;
	}

	speed_t speed;
	switch (baudrate)
	{
	case 9600:
		speed = B9600;
		break;
	case 19200:
		speed = B19200;
		break;
	case 38400:
		speed = B38400;
		break;
	case 57600:
		speed = B57600;
		break;
	case 115200:
		speed = B115200;
		break;
	default:
		speed = B9600;
		break;
	}

	if (cfsetispeed(&tio, speed) != 0
		|| cfsetospeed(&tio, speed) != 0
		|| tcsetattr(impl_->fd_, TCSANOW, &tio) != 0) {
		LOG(ERROR) << "Setup serial failed. " << port;
		close(impl_->fd_);
		impl_->fd_ = -1;
	}

	YMB_DEBUG("serial listen open success. %s,%u,%u,%u,%3.1f\n",
		port.c_str(), baudrate, databits, parity, stopbits/10);
}

std::string SerListener::GetName(void)
{
	return impl_->PeerName();
}

void SerListener::SetTimeout(long to)
{
	impl_->tv_.tv_sec = to / 1000;
	impl_->tv_.tv_usec = (to % 1000) * 1000;
}

bool SerListener::Listen(void)
{
	return impl_->fd_ != -1;
}

int SerListener::Accept(std::vector<SessionPtr> &ses)
{
	ses.clear();

	if (impl_->fd_ != -1) {
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(impl_->fd_, &fds);

		int ret = select(impl_->fd_ + 1,
			&fds, nullptr, nullptr, &impl_->tv_);
		if (ret <= 0)
			return 0;

		if (FD_ISSET(impl_->fd_, &fds)) {
			if (impl_->Recv()) {
				//data received, impl_ is the session
				ses.push_back(impl_);
			}
			return 0; //no error
		}
	}

	return -1;
}

SerListener::~SerListener()
{
	if (impl_->fd_ != -1) {
		close(impl_->fd_);
		impl_->fd_ = -1;
	}
}

} //namespace YModbus

