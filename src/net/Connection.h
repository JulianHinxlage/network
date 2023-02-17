//
// Copyright (c) 2023 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "TcpSocket.h"
#include "util/Buffer.h"
#include <thread>
#include <functional>

namespace net {

	class Connection {
	public:
		std::shared_ptr<TcpSocket> socket;
		bool outbound;
		bool packetize;

		std::function<void(Connection*, Buffer&)> readCallback;
		std::function<void(Connection*)> disconnectCallback;
		std::function<void(Connection*)> connectCallback;
		std::function<void(Connection*, ErrorCode)> errorCallback;

		Connection();
		Connection(Connection &&conn);
		~Connection();

		ErrorCode connect(const Endpoint& endpoint);
		ErrorCode connect(const std::string& address, uint16_t port, bool resolve = true, bool prefereIpv4 = false);
		void run();
		bool isRunning();
		ErrorCode write(Buffer& buffer);
		ErrorCode read(Buffer &buffer);
		void close();
		void disconnect();
	
	private:
		std::thread *thread;
		bool running;
	};

}
