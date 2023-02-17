//
// Copyright (c) 2023 Julian Hinxlage. All rights reserved.
//

#include <iostream>

#include "net/Server.h"

void main_server(const std::string& ip, int port) {
	net::Server server;
	server.packetize = true;

	server.errorCallback = [](net::Connection* conn, net::ErrorCode error) {
		printf("%s\n", net::getErrorString(error));
	};
	server.connectCallback = [](net::Connection* conn) {
		printf("connected %s:%i\n", conn->socket->getEndpoint().getAddress().c_str(), conn->socket->getEndpoint().getPort());
	};
	server.readCallback = [](net::Connection* conn, Buffer& buffer) {
		printf("> %s\n", buffer.readStr().c_str());
		buffer.reset();
		conn->write(buffer);
	};
	server.disconnectCallback = [&](net::Connection* conn) {
		printf("disconnect %s:%i\n", conn->socket->getEndpoint().getAddress().c_str(), conn->socket->getEndpoint().getPort());
	};

	net::Endpoint ep(ip, port);
	net::ErrorCode error = server.listen(port, ep.isIpv4(), false, true);
	if (error) {
		return;
	}

	server.run();

	while (server.isRunning()) {
		std::string str;
		std::getline(std::cin, str);

		if (str == "exit") {
			break;
		}
		if (str.size() == 0) {
			continue;
		}

		Buffer buffer;
		buffer.writeStr(str);

		for (auto& conn : server.connections) {
			conn->write(buffer);
		}
	}

	server.close();
}

void main_client(const std::string& ip, int port) {
	net::Server server;
	server.packetize = true;

	server.errorCallback = [](net::Connection* conn, net::ErrorCode error) {
		printf("%s\n", net::getErrorString(error));
	};
	server.connectCallback = [](net::Connection* conn) {
		printf("connected %s:%i\n", conn->socket->getEndpoint().getAddress().c_str(), conn->socket->getEndpoint().getPort());
	};
	server.readCallback = [](net::Connection* conn, Buffer& buffer) {
		printf("> %s\n", buffer.readStr().c_str());
	};
	server.disconnectCallback = [&](net::Connection* conn) {
		printf("disconnect %s:%i\n", conn->socket->getEndpoint().getAddress().c_str(), conn->socket->getEndpoint().getPort());
	};

	net::ErrorCode error = server.connectAsClient(ip, port);
	if (error) {
		return;
	}

	while (server.hasAnyConnection()) {
		std::string str;
		std::getline(std::cin, str);

		if (str == "exit") {
			break;
		}
		if (str.size() == 0) {
			continue;
		}

		Buffer buffer;
		buffer.writeStr(str);

		for (auto& conn : server.connections) {
			conn->write(buffer);
		}
	}

	server.close();
}


int main(int argc, char* argv[]) {
	std::vector<std::string> args;
	for (int i = 1; i < argc; i++) {
		args.push_back(argv[i]);
	}

	const char* usage = "[-s|-c] <ip> <port>";

	bool server = false;
	bool client = false;
	std::string ip = "::1";
	int port = 6000;
	int argNum = 0;


	for (auto& arg : args) {
		if (arg == "-s") {
			server = true;
		}
		else if (arg == "-c") {
			client = true;
		}
		else {
			if (argNum == 0) {
				ip = arg;
			}
			else if (argNum == 1) {
				try {
					port = std::stoi(arg.c_str());
				}
				catch (...) {}
			}
			else {
				printf("usage: %s\n", usage);
				return 0;
			}
			argNum++;

		}
	}

	if (!client) {
		main_server(ip, port);
	}
	else {
		main_client(ip, port);
	}

	return 0;
}
