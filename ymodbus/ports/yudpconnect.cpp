/**
* ymodbus
* Copyright © 2019-2019 liuyongqing<lyqdy1@163.com>
* v1.0.1 2019.05.04
*/
#include "ymod/master/yudpconnect.h"
#include "ymblog.h"

#ifdef WIN32
#	include <winsock2.h>
#	pragma comment(lib,"ws2_32.lib")
#else
#	include <stdio.h>
#	include <sys/types.h>
#	include <sys/socket.h>
#	include <netinet/in.h>
#   include <arpa/inet.h>
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

struct UdpConnect::Impl
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

UdpConnect::UdpConnect(const std::string &ip, uint16_t port)
	: impl_(std::make_unique<Impl>(ip, port))
{
#ifdef WIN32
	WSADATA         wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);
#endif
}

UdpConnect::~UdpConnect()
{
#if 0 //def WIN32 //We can't call this function
	WSACleanup();
#endif
}

void UdpConnect::SetTimeout(long to)
{
	impl_->tv_.tv_sec = to / 1000;
	impl_->tv_.tv_usec = (to % 1000) * 1000;
}

bool UdpConnect::Validate(void)
{
	if (impl_->sock_ != INVALID_SOCKET)
		return true;

	impl_->sock_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (impl_->sock_ == INVALID_SOCKET) {
		YMB_ERROR("Open udp socket failed!\n");
		return false;
	}

	struct sockaddr_in addr = {0};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(impl_->port_);
	addr.sin_addr.s_addr = inet_addr(impl_->ip_.c_str());

	if (connect(impl_->sock_, (SOCKADDR*)&addr, sizeof(addr)) != 0) {
		impl_->Clear();
		return false;
	}
		
	return true;
}

void UdpConnect::Purge(void)
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

bool UdpConnect::Send(uint8_t *buf, size_t len)
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

int UdpConnect::Recv(uint8_t *buf, size_t len)
{
	int ret = -ENOLINK;

	if (impl_->sock_ != INVALID_SOCKET) {
		fd_set fdsets;

		FD_ZERO(&fdsets);
		FD_SET(impl_->sock_, &fdsets);

		ret = select(impl_->sock_ + 1, &fdsets, NULL, NULL, &impl_->tv_);
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
