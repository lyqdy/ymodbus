/**
* ymodbus
* Copyright © 2019-2019 liuyongqing<lyqdy1@163.com>
* v1.0.1 2019.05.04
*/
#include "ymod/master/ytcpconnect.h"
#include "ymod/ymbutils.h"

#ifdef WIN32
#	include <winsock2.h>
#	pragma comment(lib,"ws2_32.lib")
#else
#	include <stdio.h>
#	include <sys/types.h>
#	include <sys/socket.h>
#	include <netinet/in.h>
#   include <arpa/inet.h>
#   include <unistd.h>
#	define closesocket close
#endif

namespace YModbus {

namespace {

#ifndef WIN32
	typedef int SOCKET;
	const SOCKET INVALID_SOCKET = -1;
	const int SOCKET_ERROR = -1;
#else
	typedef int socklen_t;
#endif

} //namesapce {

struct TcpConnect::Impl
{
	Impl(const std::string &ip, uint16_t port)
		: ip_(ip), port_(port), sock_(INVALID_SOCKET)
	{
	}

	~Impl()
	{
		Clear();
	}

	void Clear(void)
	{
		if (sock_ != INVALID_SOCKET) {
			closesocket(sock_);
			sock_ = INVALID_SOCKET;
		}
	}

	std::string ip_;
	uint16_t port_;
	SOCKET sock_;
	timeval tv_;
};

TcpConnect::TcpConnect(const std::string &ip, uint16_t port)
	: impl_(std::make_unique<Impl>(ip, port))
{
#ifdef WIN32
	WSADATA         wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);
#endif
}

TcpConnect::~TcpConnect()
{
#if 0 //def WIN32 //We can't call this function
	WSACleanup();
#endif
}

void TcpConnect::SetTimeout(long to)
{
	impl_->tv_.tv_sec = to / 1000;
	impl_->tv_.tv_usec = (to % 1000) * 1000;
}

bool TcpConnect::Validate(void)
{
	if (impl_->sock_ != INVALID_SOCKET)
		return true;

	impl_->sock_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (impl_->sock_ == INVALID_SOCKET) {
		return false;
	}

	struct sockaddr_in addr = {0};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(impl_->port_);
	addr.sin_addr.s_addr = inet_addr(impl_->ip_.c_str());

	if (connect(impl_->sock_, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
		impl_->Clear();
		return false;
	}

	return true;
}

void TcpConnect::Purge(void)
{
	if (impl_->sock_ != INVALID_SOCKET) {
		int ret;
		fd_set fdsets;
		timeval tv = { 0 };

		do {
			FD_ZERO(&fdsets);
			FD_SET(impl_->sock_, &fdsets);

			ret = select(impl_->sock_ + 1, &fdsets, NULL, NULL, &tv);
			if (ret > 0 && FD_ISSET(impl_->sock_, &fdsets)) {
				char buf[BUFSIZ];
				ret = recv(impl_->sock_, buf, sizeof(buf), 0);
				if (ret == 0) {
					impl_->Clear();
				}
			}
		} while (ret >= BUFSIZ);
	}
}

bool TcpConnect::Send(uint8_t *buf, size_t len)
{
	int ret;
	char *pbuf = reinterpret_cast<char *>(buf);

	if (impl_->sock_ != INVALID_SOCKET) {
		do {
			ret = send(impl_->sock_, pbuf, len, 0);
			if (ret > 0) {
				len -= static_cast<uint16_t>(ret);
				pbuf += static_cast<uint16_t>(ret);
			}
			else if (ret < 0) {
				impl_->Clear();
				return false;
			}
			else { // == 0 ?
				;//Be not imposible
			}
		} while (len != 0);
	}

	return len == 0;
}

int TcpConnect::Recv(uint8_t *buf, size_t len)
{
	int ret = -ENOLINK;

	if (impl_->sock_ != INVALID_SOCKET) {
	    struct timeval tv = impl_->tv_;
		fd_set fdsets;
		FD_ZERO(&fdsets);
		FD_SET(impl_->sock_, &fdsets);

		ret = select(impl_->sock_ + 1, &fdsets, NULL, NULL, &tv);
		if (ret > 0 && FD_ISSET(impl_->sock_, &fdsets)) {
			ret = recv(impl_->sock_, reinterpret_cast<char *>(buf), len, 0);
			if (ret == 0) {
				impl_->Clear();
				return -ECONNRESET;
			}
		}
	}

	return ret;
}

} //namespace YModbus
