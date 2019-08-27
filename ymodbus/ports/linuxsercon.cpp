/**
* ymodbus
* Copyright © 2019-2019 liuyongqing<lyqdy1@163.com>
* v1.0.1 2019.05.04
*/
#include "ymod/master/yserconnect.h"
#include "ymod/ymbdefs.h"
#include "ymod/ymbutils.h"
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

struct SerConnect::Impl
{
	Impl(const std::string &port,
		uint32_t baudrate, uint8_t databits, char parity, uint8_t stopbits)
		: fd_(-1)
		, t35_(1000000 * 12 * 20 / baudrate / 1000)
	{


        SetReadTimeout(10);

		/* Open the serial device. */
		fd_ = open(port.c_str(), O_RDWR | O_NOCTTY);
		if (fd_ < 0) {
			fd_ = -1;
			LOG(ERROR) << "Open serial failed! "
				<< port << ", "
				<< baudrate << "bps, "
				<< static_cast<int>(databits) << ", "
				<< parity << ", "
				<< std::setprecision(1) << stopbits / 10.0 << std::endl;
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
			|| tcsetattr(fd_, TCSANOW, &tio) != 0) {
			LOG(ERROR) << "Setup serial failed. " << port;
			close(fd_);
			fd_ = -1;
		}

        tcflush (fd_, TCIFLUSH);

		YMB_DEBUG("serial open success. %s,%u,%u,%c,%3.1f\n",
			port.c_str(), baudrate, databits, parity, stopbits/10.0);
	}

	bool SetReadTimeout(long to)
	{
		tv_.tv_sec = to / 1000;
		tv_.tv_usec = (to % 1000) * 1000;

        YMB_DEBUG("%s:%d %s timeout = %ld.%06ld\n",
                  __FILE__, __LINE__, __func__, tv_.tv_sec, tv_.tv_usec);

		return true;
	}

	~Impl()
	{
		if (fd_ != -1) {
			close(fd_);
			fd_ = -1;
		}
	}

	int fd_;
	struct timeval tv_;
	uint32_t t35_; //ms
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
	return impl_->fd_ != -1;
}

void SerConnect::Purge(void)
{
	if (impl_->fd_ != -1) {
	    struct timeval tv = { 0, 0 };
		fd_set fds;
		int ret;
		char buf[BUFSIZ];
		do {
			FD_ZERO(&fds);
			FD_SET(impl_->fd_, &fds);
			ret = select(impl_->fd_ + 1,
				&fds, nullptr, nullptr, &tv);
		} while (ret > 0
			&& FD_ISSET(impl_->fd_, &fds)
			&& read(impl_->fd_, buf, static_cast<int>(sizeof(buf))) > 0);
	}
}

bool SerConnect::Send(uint8_t *buf, size_t len)
{
	if (impl_->fd_ != -1) {
		char *pbuf = reinterpret_cast<char *>(buf);
		int ret;
		do {
			ret = write(impl_->fd_, pbuf, len);
			if (ret > 0) {
			    YMB_HEXDUMP0(pbuf, ret, "%p SerConnect::Send: ", this);
				len -= static_cast<size_t>(ret);
				pbuf += static_cast<size_t>(ret);
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
    int recvlen = 0;

 	if (impl_->fd_ != -1) {
	    struct timeval tv = impl_->tv_;
		fd_set fds;
        int ret;

		do {
            FD_ZERO(&fds);
            FD_SET(impl_->fd_, &fds);

            ret = select(impl_->fd_ + 1, &fds, nullptr, nullptr, &tv);
            if (ret > 0 && FD_ISSET(impl_->fd_, &fds)) {
                ret = read(impl_->fd_,
                            reinterpret_cast<char*>(buf) + recvlen,
                            static_cast<int>(len) - recvlen);
                if (ret > 0) {
                    YMB_HEXDUMP0(buf + recvlen, ret, "%p SerConnect::Recv: ", this);
                    recvlen += ret;
                }
            }
            else {
                YMB_DEBUG0("SerConnect::Recv recvlen = %d, ret = %d\n",
                          recvlen, ret);
            }

            tv.tv_sec = impl_->t35_ / 1000;
            tv.tv_usec = impl_->t35_ % 1000 * 1000;
		} while (ret > 0 && recvlen < static_cast<int>(len));
	}

	return recvlen;
}

} //namespace YModbus
