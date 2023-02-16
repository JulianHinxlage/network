//
// Copyright (c) 2023 Julian Hinxlage. All rights reserved.
//

#include <iostream>
#include <vector>
#include <string>
#include <thread>

#include "net/TcpSocket.h"

void main_server(const std::string &ip, int port) {
	net::TcpSocket listener;
	net::Endpoint ep(ip, port);
	net::ErrorCode error;
	std::vector<std::thread*> threads;

	error = listener.listen(port, ep.isIpv4(), false, true);
	if (error) {
		printf("%s\n", net::getErrorString(error));
		return;
	}

	while (true) {
		auto socket = listener.accept();
		if (!socket) {
			break;
		}

		printf("connetced %s:%i\n", socket->getEndpoint().getAddress().c_str(), socket->getEndpoint().getPort());

		threads.push_back(new std::thread([socket]() {
			char buffer[1024];
			net::ErrorCode error;
			while (socket->isConnected()) {
				int len = sizeof(buffer) - 1;

				error = socket->read(buffer, len);
				if (error) {
					printf("%s\n", net::getErrorString(error));
					break;
				}
				buffer[len] = '\0';
				printf("> %s\n", buffer);

				error = socket->write(buffer, len);
				if (error) {
					printf("%s\n", net::getErrorString(error));
					break;
				}
			}

			printf("disconnect %s:%i\n", socket->getEndpoint().getAddress().c_str(), socket->getEndpoint().getPort());
		}));
	}

	listener.disconnect();
}

void main_client(const std::string& ip, int port) {
	net::TcpSocket socket;
	net::ErrorCode error;
	
	error = socket.connect(ip, port);
	if (error) {
		printf("%s\n", net::getErrorString(error));
		return;
	}

	printf("connetced %s:%i\n", socket.getEndpoint().getAddress().c_str(), socket.getEndpoint().getPort());

	char buffer[1024];
	while (socket.isConnected()) {
		std::string str;
		std::getline(std::cin, str);

		if (str == "exit") {
			break;
		}
		if (str.size() == 0) {
			continue;
		}

		error = socket.write(str.c_str(), str.size());
		if (error) {
			printf("%s\n", net::getErrorString(error));
			return;
		}

		int len = sizeof(buffer) - 1;
		error = socket.read(buffer, len);
		if (error) {
			printf("%s\n", net::getErrorString(error));
			return;
		}

		buffer[len] = '\0';
		printf("> %s\n", buffer);
	}

	socket.disconnect();
}

int main(int argc, char *argv[]) {
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
			else if (argNum == 1){
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
