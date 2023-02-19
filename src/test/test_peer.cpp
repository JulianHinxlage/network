//
// Copyright (c) 2023 Julian Hinxlage. All rights reserved.
//

#include <iostream>
#include <vector>
#include <string>
#include <thread>

#include "peer/PeerNetwork.h"
#include "util/hex.h"
#include "util/strutil.h"

int main(int argc, char* argv[]) {
	std::vector<std::string> args;
	for (int i = 1; i < argc; i++) {
		args.push_back(argv[i]);
	}

	bool clientOnly = false;
	for (auto& arg : args) {
		if (arg == "-c") {
			clientOnly = true;
		}
	}

	net::PeerNetwork network;

	network.logVerbosity = 3;
	network.logCallback = [](const char* str) {
		printf("%s", str);
	};

	network.readCallback = [&](net::PeerId id, Buffer &buffer) {
		printf("%s: %s\n", network.idToStr(id).c_str(), buffer.readStr().c_str());
	};

	if (!network.loadConfigFile("peer.cfg")) {
		if (!network.loadConfigFile("../peer.cfg")) {
			if (!network.loadConfigFile("../../peer.cfg")) {
				printf("config file peer.cfg not found\n");
				network.setLocalPeer("localhost", 6000, 32);
				for (int i = 0; i < 10; i++) {
					network.addEntryNode("localhost", 6000 + i);
				}
			}
		}
	}

	network.connect(clientOnly);

	while (true) {
		std::string str;
		std::getline(std::cin, str);
		if(str == "exit") {
			break;
		}
		else if (str == "ping") {
			network.broadcastPing();
			continue;
		}
		else if (str == "state") {
			printf("state: %i\n", network.getState());
			continue;
		}

		Buffer buffer;
		buffer.writeStr(str);
		network.broadcast(buffer);
	}

	network.disconnect();
	return 0;
}
