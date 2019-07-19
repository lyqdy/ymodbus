/**
* ymodbus
* Copyright © 2019-2019 liuyongqing<lyqdy1@163.com>
* v1.0.1 2019.05.04
*/
#include "ymod/slave/yudplistener.h"
#include "ymod/ymbdefs.h"
#include "ymbopts.h"
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

struct UdpListener::Impl : public ISession
{
	Impl(uint16_t port)
		: port_(port)
		, recvlen_(0) 
		, alen_(sizeof(addr_))
	{
	}

	virtual std::string PeerName(void);
	virtual int Write(uint8_t *msg, size_t msglen);
	virtual int Read(uint8_t *buf, size_t bufsiz);
	virtual size_t Peek(uint8_t **buf);

	virtual void Purge(void);
	virtual void Discard(size_t nbytes);

	bool Recv(void);

	uint16_t port_;
	SOCKET sock_;
	timeval tv_ = { 0, 0 };
	char recvbuf_[kMaxMsgLen];
	size_t recvlen_;
	sockaddr addr_;
	socklen_t alen_;
};

std::string UdpListener::Impl::PeerName()
{
	struct sockaddr_in addr;
	int addrlen = sizeof(addr);

	std::string peer = "tcp:";
	peer += std::to_string(sock_);
	peer += ":connect:";

	if (getpeername(sock_,
		reinterpret_cast<sockaddr*>(&addr), &addrlen) == 0) {
		peer = inet_ntoa(addr.sin_addr);
		peer += ":";
		peer += std::to_string(ntohs(addr.sin_port));
	}
	else {
		peer += "error peer";
	}

	return peer;
}

int UdpListener::Impl::Write(uint8_t *msg, size_t msglen)
{
	if (sock_ != INVALID_SOCKET) {
		YMB_HEXDUMP0(msg, msglen, "udp sendto sock = %d", sock_);
		return sendto(sock_, (char *)msg, (int)msglen, 0, &addr_, alen_);
	}

	return -1;
}

int UdpListener::Impl::Read(uint8_t *buf, size_t bufsiz)
{
	if (recvlen_ < bufsiz)
		bufsiz = recvlen_;

	memcpy(buf, recvbuf_, bufsiz);
	recvlen_ -= bufsiz;

	return static_cast<int>(bufsiz);
}

size_t UdpListener::Impl::Peek(uint8_t **buf)
{
	*buf = reinterpret_cast<uint8_t*>(&recvbuf_[0]);
	return recvlen_;
}

void UdpListener::Impl::Purge(void)
{
	recvlen_ = 0;
}

void UdpListener::Impl::Discard(size_t nbytes)
{
	if (recvlen_ > nbytes) {
		recvlen_ -= nbytes;
		memmove(recvbuf_, recvbuf_ + nbytes, recvlen_);
	}
	else {
		recvlen_ = 0;
	}
}

bool UdpListener::Impl::Recv()
{
	int len = recvfrom(sock_, recvbuf_, sizeof(recvbuf_), 0, &addr_, &alen_);
	if (len > 0) {
		recvlen_ = len;
		YMB_HEXDUMP0(recvbuf_, recvlen_,
			"%s recvbuf, len = %u ", PeerName().c_str(), recvlen_);
		return true;
	}

	recvlen_ = 0;

	return false;
}

UdpListener::UdpListener(uint16_t port)
	: impl_(std::make_shared<Impl>(port))
{
#ifdef WIN32
	WSADATA         wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);
#endif

	impl_->sock_ = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (impl_->sock_ == INVALID_SOCKET) {
		YMB_ERROR("Open udp socket failed. port = %u\n ", port);
		return;
	}

	int opt = 1;
	setsockopt(impl_->sock_,
		SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;

	int ret = bind(impl_->sock_, (struct sockaddr *)&addr, sizeof(addr));
	if (ret == SOCKET_ERROR) {
		YMB_ERROR("Udp socket bind failed. port = %u\n", port);
		closesocket(impl_->sock_);
		impl_->sock_ = INVALID_SOCKET;
		return;
	}

	YMB_DEBUG("Open udp listen success. port = %u\n", port);
}

UdpListener::~UdpListener()
{
	if (impl_->sock_ != INVALID_SOCKET) {
		closesocket(impl_->sock_);
		impl_->sock_ = INVALID_SOCKET;
	}

#if 0 //def WIN32 //We needn't call this function
	WSACleanup();
#endif
}

std::string UdpListener::GetName(void)
{
	struct sockaddr_in addr;
	int addrlen = sizeof(addr);

	std::string peer = "udp:";
	peer += std::to_string(impl_->sock_);
	peer += ":listen:";

	if (getsockname(impl_->sock_,
		reinterpret_cast<sockaddr*>(&addr), &addrlen) == 0) {
		peer = inet_ntoa(addr.sin_addr);
		peer += ":";
		peer += std::to_string(ntohs(addr.sin_port));
	}
	else {
		peer += "error sock";
	}

	return peer;
}

void UdpListener::SetTimeout(long to)
{
	impl_->tv_.tv_sec = to / 1000;
	impl_->tv_.tv_usec = (to % 1000) * 1000;
}

bool UdpListener::Listen(void)
{
	return impl_->sock_ != INVALID_SOCKET;
}

int UdpListener::Accept(std::vector<SessionPtr> &ses)
{
	ses.clear();

	if (impl_->sock_ != INVALID_SOCKET) {
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(impl_->sock_, &fds);

		int ret = select(impl_->sock_ + 1,
			&fds, nullptr, nullptr, &impl_->tv_);
		if (ret <= 0)
			return EOK;

		if (FD_ISSET(impl_->sock_, &fds)) {
			if (impl_->Recv()) {
				//data received, impl_ is the session
				ses.push_back(impl_);
			}
			return EOK; //no error
		}
	}

	return -1;
}

} //namespace YModbus

