//
// Copyright (c) 2023 Julian Hinxlage. All rights reserved.
//

#include <iostream>
#include <vector>
#include <string>
#include <thread>

#include "net/UdpSocket.h"

void main_server(const std::string& ip, int port) {
	net::UdpSocket socket;
	net::Endpoint ep(ip, port);
	net::ErrorCode error;

	error = socket.listen(port, ep.isIpv4(), false, true);
	if (error) {
		printf("%s\n", net::getErrorString(error));
		return;
	}

	while (true) {
		char buffer[1024];
		int len = sizeof(buffer) - 1;

		error = socket.read(buffer, len, ep);
		if (error) {
			printf("%s\n", net::getErrorString(error));
			return;
		}

		buffer[len] = '\0';
		printf("> %s\n", buffer);
		error = socket.write(buffer, len, ep);
		if (error) {
			printf("%s\n", net::getErrorString(error));
			return;
		}
	}

	socket.close();
}

void main_client(const std::string& ip, int port) {
	net::UdpSocket socket;
	net::Endpoint ep(ip, port);
	net::ErrorCode error;

	error = socket.create(ep.isIpv4());
	if (error) {
		printf("%s\n", net::getErrorString(error));
		return;
	}

	char buffer[1024];
	while (true) {

		std::string str;
		std::getline(std::cin, str);

		if (str == "exit") {
			break;
		}
		if (str.size() == 0) {
			continue;
		}

		error = socket.write(str.c_str(), str.size(), ep);
		if (error) {
			printf("%s\n", net::getErrorString(error));
			return;
		}

		int len = sizeof(buffer) - 1;
		error = socket.read(buffer, len, ep);
		if (error) {
			printf("%s\n", net::getErrorString(error));
			return;
		}
		buffer[len] = '\0';
		printf("> %s\n", buffer);
	}

	socket.close();
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
