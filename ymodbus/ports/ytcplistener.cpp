/**
* ymodbus
* Copyright © 2019-2019 liuyongqing<lyqdy1@163.com>
* v1.0.1 2019.05.04
*/
#include "ymod/slave/ytcplistener.h"
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

#include <ctime>
#include <algorithm>
#include <memory>

namespace YModbus {

namespace {

#ifndef WIN32
	typedef int SOCKET;
	const SOCKET INVALID_SOCKET = -1;
	const int SOCKET_ERROR = -1;
#else
	typedef int socklen_t;
#endif

const int kTcpListenNum = 5;
const time_t kMinIdleTime = 300; //s

struct TcpSession : public ISession
{
	TcpSession(SOCKET sock)
		: sock_(sock)
		, lrt_(time(0))
		, recvlen_(0)
	{
	}

	~TcpSession()
	{
		Reset(INVALID_SOCKET);
	}

	void Reset(SOCKET sock)
	{
		YMB_DEBUG("Tcp socket closed.  socket = %d\n", sock_);
		closesocket(sock_);
		sock_ = sock;
		recvlen_ = 0;
		lrt_ = time(0);
	}

	virtual std::string PeerName(void);
	virtual int Write(uint8_t *msg, size_t msglen);
	virtual int Read(uint8_t *buf, size_t bufsiz);
	virtual size_t Peek(uint8_t **buf);

	virtual void Purge(void);
	virtual void Discard(size_t nbytes);

	bool Recv(void);

	SOCKET sock_;
	time_t lrt_; //last recv msg time
	char recvbuf_[kMaxMsgLen];
	size_t recvlen_;
};

std::string TcpSession::PeerName()
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

int TcpSession::Write(uint8_t *msg, size_t msglen)
{
	int len = static_cast<int>(msglen);
	char *pbuf = reinterpret_cast<char*>(msg);
	int sentlen = 0;

	do {
		sentlen = send(sock_, pbuf, len, 0);
		if (sentlen > 0) {
			pbuf += sentlen;
			len -= sentlen;
			YMB_HEXDUMP0(pbuf, sentlen,
				"%s write data, len = %u", PeerName().c_str(), sentlen);
		}
	} while (len > 0 && sentlen >= 0);

	return len == 0 ? EOK : -EFAULT;
}

int TcpSession::Read(uint8_t *buf, size_t bufsiz)
{
	if (recvlen_ < bufsiz)
		bufsiz = recvlen_;

	memcpy(buf, recvbuf_, bufsiz);
	recvlen_ -= bufsiz;

	return static_cast<int>(bufsiz);
}

size_t TcpSession::Peek(uint8_t **buf)
{
	*buf = reinterpret_cast<uint8_t*>(&recvbuf_[0]);

	return recvlen_;
}

void TcpSession::Purge(void)
{
	fd_set fds;
	timeval tv = { 0, 0 };
	int ret;

	do {
		FD_ZERO(&fds);
		FD_SET(sock_, &fds);

		ret = select(sock_ + 1, &fds, nullptr, nullptr, &tv);
		if (ret > 0) {
			if (FD_ISSET(sock_, &fds))
				ret = recv(sock_, recvbuf_, sizeof(recvbuf_), 0);
			else
				ret = 0;
		}
	} while (ret > 0);

	recvlen_ = 0;
}

void TcpSession::Discard(size_t nbytes)
{
	if (recvlen_ > nbytes) {
		recvlen_ -= nbytes;
		memmove(recvbuf_, recvbuf_ + nbytes, recvlen_);
	}
	else {
		recvlen_ = 0;
	}
}

bool TcpSession::Recv()
{
	if (recvlen_ == sizeof(recvbuf_))
		recvlen_ = 0;

	int len = recv(sock_,
		&recvbuf_[recvlen_], sizeof(recvbuf_) - recvlen_, 0);

	if (len > 0) {
		recvlen_ += len;
		lrt_ = time(0);
		YMB_HEXDUMP0(recvbuf_, recvlen_,
			"%s recvbuf, len = %u ", PeerName().c_str(), recvlen_);
		return true;
	}

	return false;
}

} //namespace {

struct TcpListener::Impl
{
	typedef std::shared_ptr<TcpSession> TcpSessionPtr;

	timeval tv_ = { 0, 0 };
	SOCKET sock_ = INVALID_SOCKET; //listen sockets
	std::vector<TcpSessionPtr> ses_;

	bool AddSession(SOCKET sock)
	{
		if (ses_.size() < kMaxTcpSessionNum) {
			ses_.push_back(std::make_shared<TcpSession>(sock));
			return true;
		}

		auto session = *std::min_element(ses_.begin(), ses_.end(),
			[](TcpSessionPtr s1, TcpSessionPtr s2) {
			return s1->lrt_ < s2->lrt_; //min lrt
		});

		time_t now = time(0);
		if (now < session->lrt_ || now - session->lrt_ > kMinIdleTime) {
			session->Reset(sock); //reuse session
			return true; //ok, found a idle session
		}

		return false; //idle session object not found! connect refused.
	}
};

TcpListener::TcpListener(uint16_t port)
	: impl_(std::make_unique<Impl>())
{
#ifdef WIN32
	WSADATA         wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);
#endif

	impl_->sock_ = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (impl_->sock_ == INVALID_SOCKET) {
		YMB_ERROR("Open listen socket failed. port = %u\n ", port);
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
		YMB_ERROR("Tcp socket bind failed. port = %u\n", port);
		closesocket(impl_->sock_);
		impl_->sock_ = INVALID_SOCKET;
		return;
	}

	YMB_DEBUG("Open tcp listen success. port = %u\n", port);
}

TcpListener::~TcpListener()
{
	if (impl_->sock_ != INVALID_SOCKET) {
		closesocket(impl_->sock_);
		impl_->sock_ = INVALID_SOCKET;
	}

#if 0 //def WIN32 //We needn't call this function
	WSACleanup();
#endif
}

std::string TcpListener::GetName(void)
{
	struct sockaddr_in addr;
	int addrlen = sizeof(addr);

	std::string peer = "tcp:";
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

void TcpListener::SetTimeout(long to)
{
	impl_->tv_.tv_sec = to / 1000;
	impl_->tv_.tv_usec = (to % 1000) * 1000;
}

bool TcpListener::Listen(void)
{
	if (impl_->sock_ == INVALID_SOCKET)
		return false;

	int ret = listen(impl_->sock_, kTcpListenNum);
	if (ret == SOCKET_ERROR) {
		YMB_ERROR("Tcp listen socket listen failed.\n");
		closesocket(impl_->sock_);
		impl_->sock_ = INVALID_SOCKET;
		return false;
	}

	return true;
}

int TcpListener::Accept(std::vector<SessionPtr> &ses)
{
	ses.clear();

	SOCKET maxsock = -1;
	fd_set fds;
	FD_ZERO(&fds);

	if (impl_->sock_ != INVALID_SOCKET) {
		FD_SET(impl_->sock_, &fds);
		maxsock = impl_->sock_;
	}

	for (const auto &s : impl_->ses_) {
		FD_SET(s->sock_, &fds);
		if (s->sock_ > maxsock)
			maxsock = s->sock_;
	}

	int ret = select(maxsock + 1, &fds, nullptr, nullptr, &impl_->tv_);
	if (ret <= 0)
		return EOK;

	//session socket
	for (auto it = impl_->ses_.begin(); it != impl_->ses_.end();) {
		if (FD_ISSET((*it)->sock_, &fds)) {
			if ((*it)->Recv()) { //data arrived
				ses.push_back(*it);
				++it;
			}
			else { //error or closed
				it = impl_->ses_.erase(it);
			}
		}
		else {	//!FD_ISSET
			++it;
		}
	}

	//Listen socket
	if (FD_ISSET(impl_->sock_, &fds)) {
		sockaddr addr;
		socklen_t addrlen = sizeof(addr);
		SOCKET sock = accept(impl_->sock_, &addr, &addrlen);
		if (sock != SOCKET_ERROR) {
			sockaddr_in *addrin = (sockaddr_in*)&addr;
			YMB_DEBUG("New tcp connect. socket = %d, ip = %s, port = %u\n",
				sock, inet_ntoa(addrin->sin_addr), ntohs(addrin->sin_port));
			if (!impl_->AddSession(sock)) {
				YMB_ERROR("Idle session object not found!connect refused."
					"socket = %d, ip = %s, port = %u\n", sock, 
					inet_ntoa(addrin->sin_addr), ntohs(addrin->sin_port));
				closesocket(sock);
			}
		}
	}

	return EOK;
}

} //namespace YModbus

