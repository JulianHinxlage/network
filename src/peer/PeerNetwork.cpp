//
// Copyright (c) 2023 Julian Hinxlage. All rights reserved.
//

#include "PeerNetwork.h"
#include "util/random.h"
#include "util/hex.h"
#include "util/strutil.h"
#include <stdarg.h>

namespace net {

	const char* getOpcodeName(PeerNetwork::Opcode opcode) {
		switch (opcode)
		{
		case net::PeerNetwork::NONE:
			return "NONE";
		case net::PeerNetwork::PING:
			return "PING";
		case net::PeerNetwork::PONG:
			return "PONG";
		case net::PeerNetwork::HANDSHAKE:
			return "HANDSHAKE";
		case net::PeerNetwork::HANDSHAKE_REPLY:
			return "HANDSHAKE_REPLY";
		case net::PeerNetwork::LOOKUP:
			return "LOOKUP";
		case net::PeerNetwork::LOOKUP_REPLY:
			return "LOOKUP_REPLY";
		case net::PeerNetwork::ROUTE:
			return "ROUTE";
		case net::PeerNetwork::MESSAGE:
			return "MESSAGE";
		case net::PeerNetwork::BROADCAST:
			return "BROADCAST";
		case net::PeerNetwork::DISCONNECT:
			return "DISCONNECT";
		default:
			return "INVALID_OPCODE";
		}
	}

	const char* getStateName(PeerNetwork::State state) {
		switch (state)
		{
		case net::PeerNetwork::DISCONNECTED:
			return "DISCONNECTED";
		case net::PeerNetwork::CONNECTING_TO_ENTRY_NODE:
			return "CONNECTING_TO_ENTRY_NODE";
		case net::PeerNetwork::SENDING_LOOKUPS:
			return "SENDING_LOOKUPS";
		case net::PeerNetwork::LOOKUPS_SEND:
			return "LOOKUPS_SEND";
		case net::PeerNetwork::CONNECTED:
			return "CONNECTED";
		default:
			return "INVALID_STATE";
		}
	}

	PeerNetwork::PeerNetwork() {
		clientOnly = false;
	}

	PeerNetwork::~PeerNetwork() {
		if (lookupThread) {
			lookupThread->join();
			lookupThread = nullptr;
		}
	}

	void PeerNetwork::addEntryNode(const std::string& address, uint16_t port) {
		Peer peer;
		peer.address = address;
		peer.port = port;
		entryNodes.push_back(peer);
	}

	bool PeerNetwork::loadConfigFile(const std::string& file) {
		std::string config = strReadFile(file);
		if (config.empty()) {
			return false;
		}

		auto lines = strSplit(config, "\n", false);
		for (auto& line : lines) {
			auto parts = strSplit(line, " ", false);

			if (parts.size() > 0) {
				if (parts[0] == "local") {
					std::string address = "localhost";
					int port = 6000;
					int portOffset = 0;
					PeerId id = PeerId(0);

					if (parts.size() > 1) {
						address = parts[1];
					}
					if (parts.size() > 2) {
						try {
							port = std::stoi(parts[2]);
						}
						catch (...) {}
					}
					if (parts.size() > 3) {
						try {
							portOffset = std::stoi(parts[3]);
						}
						catch (...) {}
					}
					if (parts.size() > 4) {
						std::string hex = parts[4];
						while (hex.size() < sizeof(PeerId) * 2) {
							hex.push_back('0');
						}
						swapEndianness(hex);
						fromHex(hex, id);
					}

					if (parts.size() > 1) {
						setLocalPeer(address, port, portOffset, id);
					}
				}
				else if (parts[0] == "entry") {
					std::string address = "localhost";
					int port = 6000;

					if (parts.size() > 1) {
						address = parts[1];
					}
					if (parts.size() > 2) {
						try {
							port = std::stoi(parts[2]);
						}
						catch (...) {}
					}

					if (parts.size() > 1) {
						addEntryNode(address, port);
					}
				}
			}
		}
		
		return true;
	}

	void PeerNetwork::connect(bool clientOnly) {
		log(1, "local id: %s\n", idToStr(localId).c_str());
		uint16_t localPort = routingTable.localPeer->port;

		this->clientOnly = clientOnly;
		server.packetize = true;
		wasEntryNodeLookedUp = false;
		setState(State::DISCONNECTED);
		entryNode = nullptr;

		server.errorCallback = [&](Connection* conn, ErrorCode error) {
			log(3, "%s\n", getErrorString(error));
		};
		server.connectCallback = [&](Connection* conn) {
			log(1, "connection %s %i outbound=%i\n", conn->socket->getEndpoint().getAddress().c_str(), conn->socket->getEndpoint().getPort(), conn->outbound);
			onConnect(conn);
		};
		server.disconnectCallback = [&](Connection* conn) {
			log(1, "disconnect %s %i\n", conn->socket->getEndpoint().getAddress().c_str(), conn->socket->getEndpoint().getPort());
			onDisconnect(conn);
		};
		server.readCallback = [&](Connection* conn, Buffer &buffer) {
			Peer* peer = routingTable.get(conn);
			processPacket(peer, buffer, peer->id, true);
		};

		if (!clientOnly) {
			//listen on local port
			for (int i = 0; i <= maxPortOffset; i++) {
				routingTable.localPeer->port = localPort;
				ErrorCode error = server.listen(localPort, false, false, true);
				if (error) {
					localPort++;
					continue;
				}
				else {
					log(1, "listening on port %i\n", localPort);
				}
				break;
			}

			server.run();
		}

		//connect to entry node
		setState(State::CONNECTING_TO_ENTRY_NODE);
		for (auto& node : entryNodes) {
			Endpoint ep(node.address, node.port);
			if (node.address != routingTable.localPeer->address || node.port != localPort) {
				if (ep.isValid() && (ep.getAddress() != routingTable.localPeer->address || ep.getPort() != localPort)) {
					if (getState() != State::CONNECTING_TO_ENTRY_NODE) {
						break;
					}
					log(2, "try connecting to %s %i\n", node.address.c_str(), node.port);
					if (connectToPeer(ep.getAddress(), ep.getPort())) {
						break;
					}
					if (getState() != State::CONNECTING_TO_ENTRY_NODE) {
						break;
					}
				}
			}
		}
	}

	void PeerNetwork::setLocalPeer(const std::string& localAddress, uint16_t localPort, int maxPortOffset, PeerId id) {
		routingTable.localPeer->address = localAddress;
		routingTable.localPeer->port = localPort;
		this->maxPortOffset = maxPortOffset;
		if (id == PeerId(0)) {
			randomBytes(localId);
		}
		else {
			localId = id;
		}
		routingTable.localPeer->id = localId;
	}

	void PeerNetwork::disconnect() {
		server.close();
		setState(State::DISCONNECTED);
	}

	bool PeerNetwork::isConnected() {
		for (auto& peer : routingTable.peers) {
			if (peer && peer->conn) {
				if (peer->conn->socket->isConnected()) {
					if (peer->state == Peer::CONNECTED) {
						return true;
					}
				}
			}
		}
		return false;
	}

	PeerId PeerNetwork::getLocalId() {
		return localId;
	}

	PeerId PeerNetwork::getRandomNeighborPeer() {
		int i = 0;
		randomBytes(i);
		int index = i % routingTable.peers.size();
		return routingTable.peers[index]->id;
	}

	PeerNetwork::State PeerNetwork::getState() {
		return state;
	}

	void PeerNetwork::send(PeerId id, Buffer& payload, bool exact) {
		Buffer packet;
		createPacketRoute(packet, localId, id, 1);
		packet.write(Opcode::MESSAGE);
		packet.writeBytes(payload.data(), payload.size());

		sendToNextPeer(id, packet);
	}

	void PeerNetwork::broadcast(Buffer& payload) {
		Buffer packet;
		packet.write(Opcode::BROADCAST);
		packet.write(localId);
		uint64_t nonce;
		randomBytes(nonce);
		packet.write(nonce);

		packet.write(Opcode::MESSAGE);
		packet.writeBytes(payload.data(), payload.size());

		sendToAllPeers(packet);
	}

	void PeerNetwork::broadcastPing() {
		Buffer packet;
		packet.write(Opcode::BROADCAST);
		packet.write(localId);
		uint64_t nonce;
		randomBytes(nonce);
		packet.write(nonce);

		packet.write(Opcode::PING);

		sendToAllPeers(packet);
	}

	bool PeerNetwork::connectToPeer(const std::string& address, uint16_t port) {
		ErrorCode error = server.connectAsClient(address, port);
		return !error;
	}

	void PeerNetwork::disconnectFromPeer(Peer* peer) {
		Buffer reply;
		reply.write(Opcode::DISCONNECT);
		peer->conn->write(reply);
		peer->conn->disconnect();
	}

	void PeerNetwork::performLookups() {
		if (lookupThread) {
			lookupThread->join();
			lookupThread = nullptr;
		}

		lookupThread = std::make_shared<std::thread>([&]() {
			for (int i = 0; i < lookupCountOnConnect; i++) {
				std::this_thread::sleep_for(std::chrono::milliseconds(20));
				auto target = routingTable.getLookupTarget(i);

				if (entryNode) {
					Buffer packet;
					createPacketRoute(packet, localId, target, 0);
					createPacketLookup(packet, localId, entryNode->id, target);
					lookupTargets.insert(target);
					if (i == lookupCountOnConnect - 1) {
						setState(State::LOOKUPS_SEND);
					}
					entryNode->conn->write(packet);
				}
			}
		});
	}

	void PeerNetwork::onConnect(Connection* conn) {
		Peer peer;
		peer.conn = conn;
		peer.address = conn->socket->getEndpoint().getAddress();
		peer.port = conn->socket->getEndpoint().getPort();
		peer.state = Peer::PRE_HANDSHAKE;

		routingTable.add(peer);

		if (conn->outbound) {
			Buffer packet;
			createPacketHandshake(packet, localId, routingTable.localPeer->port, routingTable.localPeer->address);
			conn->write(packet);
		}

		if (getState() == State::CONNECTING_TO_ENTRY_NODE) {
			entryNode = routingTable.get(conn);
		}
	}

	void PeerNetwork::onDisconnect(Connection* conn) {
		Peer* peer = routingTable.get(conn);
		if (!peer) {
			return;
		}
		peer->state = Peer::DISCONNECTED;
		PeerId id = peer->id;
		routingTable.remove(conn);

		if (server.isRunning()) {
			int index = routingTable.getLookupIndex(id);
			PeerId target = routingTable.getLookupTarget(index);
			Buffer packet;
			createPacketRoute(packet, localId, target, 0);
			createPacketLookup(packet, localId, localId, target);

			Peer* next = routingTable.getNext(target, id, false);
			if (next) {
				lookupTargets.insert(target);
				next->conn->write(packet);
			}
		}
	}

	void PeerNetwork::processPacket(Peer* peer, Buffer& packet, PeerId routingSource, bool wasSendDirectly) {
		if (!peer) {
			return;
		}

		if (debugFakeLatency && wasSendDirectly) {
			if (getState() == State::CONNECTED) {
				uint32_t time;
				randomBytes(time);
				time = time % 1000;
				std::this_thread::sleep_for(std::chrono::milliseconds(time));
			}
		}

		int startReadIndex = packet.getReadIndex();
		Opcode opcode = packet.read<Opcode>();

		log(4, "packet: %s\n", getOpcodeName(opcode));

		switch (opcode) {
		case PING: {
			Buffer reply;
			createPacketRoute(reply, localId, routingSource, 1);
			reply.write(Opcode::PONG);
			peer->conn->write(reply);
			break;
		}
		case PONG: {
			log(2, "ping: %s\n", idToStr(routingSource).c_str());
			break;
		}
		case HANDSHAKE: {
			if (wasSendDirectly) {
				peer->id = packet.read<PeerId>();
				peer->port = packet.read<uint16_t>();
				peer->address = packet.readStr();
				peer->state = Peer::CONNECTED;

				log(4, "handshake %s %i %s\n", idToStr(peer->id).c_str(), peer->port, peer->address.c_str());

				Buffer reply;
				reply.write(Opcode::HANDSHAKE_REPLY);
				reply.write(localId);
				reply.write(routingTable.localPeer->port);
				reply.writeStr(routingTable.localPeer->address);
				peer->conn->write(reply);


				if (getState() == State::CONNECTING_TO_ENTRY_NODE) {
					if (peer == entryNode) {
						setState(State::CONNECTED);
					}
				}
				break;
			}
		}
		case HANDSHAKE_REPLY: {
			if (wasSendDirectly) {
				peer->id = packet.read<PeerId>();
				peer->port = packet.read<uint16_t>();
				peer->address = packet.readStr();
				peer->state = Peer::CONNECTED;

				log(4, "handshake reply %s %i %s\n", idToStr(peer->id).c_str(), peer->port, peer->address.c_str());

				if (getState() == State::CONNECTING_TO_ENTRY_NODE) {
					if (peer == entryNode) {
						if (clientOnly) {
							setState(State::CONNECTED);
						}
						else {
							setState(State::SENDING_LOOKUPS);
							performLookups();
						}
					}
				}
			}
			break;
		}
		case LOOKUP: {
			PeerId source = packet.read<PeerId>();
			PeerId relay = packet.read<PeerId>();
			PeerId target = packet.read<PeerId>();

			log(4, "lookup %s %s %s\n", idToStr(source).c_str(), idToStr(relay).c_str(), idToStr(target).c_str());

			Buffer reply;
			if (relay != localId) {
				createPacketRoute(reply, localId, relay, 1);
			}
			createPacketRoute(reply, localId, source, 1);
			createPacketLookupReply(reply, localId, target, routingTable.localPeer->port, routingTable.localPeer->address);
			peer->conn->write(reply);
			break;
		}
		case LOOKUP_REPLY: {
			PeerId source = packet.read<PeerId>();
			PeerId target = packet.read<PeerId>();
			uint16_t port = packet.read<uint16_t>();
			std::string address = packet.readStr();

			log(4, "lookup reply %s %s %i %s\n", idToStr(source).c_str(), idToStr(target).c_str(), port, address.c_str());

			if (entryNode) {
				if (source == entryNode->id) {
					wasEntryNodeLookedUp = true;
				}
			}


			if (!lookupReplyTargets.contains(source)) {
				lookupReplyTargets.insert(source);
				routingTableMutex.lock();
				if (!routingTable.has(source) && source != localId) {
					connectToPeer(address, port);
				}
				routingTableMutex.unlock();
			}

			lookupTargets.erase(target);
			if (entryNode) {
				if (lookupTargets.empty()) {
					if (getState() == State::LOOKUPS_SEND) {
						if (!wasEntryNodeLookedUp) {
							disconnectFromPeer(entryNode);
							entryNode = nullptr;
						}
						setState(State::CONNECTED);
					}
				}
			}

			break;
		}
		case ROUTE: {
			PeerId source = packet.read<PeerId>();
			PeerId target = packet.read<PeerId>();
			uint8_t exact = packet.read<uint8_t>();

			log(4, "route %s %s %i\n", idToStr(source).c_str(), idToStr(target).c_str(), exact);

			Peer *next = routingTable.getNext(target, peer->id, true);
			if (next == routingTable.localPeer.get()) {
				if (!exact || target == localId) {
					processPacket(peer, packet, source, false);
				}
				else {
					log(3, "packt dropped\n");
				}
			}
			else {
				if (next) {
					packet.unskip(packet.getReadIndex() - startReadIndex);
					next->conn->write(packet);
				}
				else {
					log(3, "packt dropped\n");
				}
			}

			break;
		}
		case MESSAGE: {
			if (readCallback) {
				readCallback(routingSource, packet);
			}
			break;
		}
		case BROADCAST: {
			PeerId source = packet.read<PeerId>();
			uint64_t nonce = packet.read<uint64_t>();

			if (!seenBroadcastNonces.contains(nonce)) {
				seenBroadcastNonces.insert(nonce);
				if (source != localId) {
					int endReadIndex = packet.getReadIndex();
					packet.unskip(packet.getReadIndex() - startReadIndex);

					sendToAllPeers(packet, peer->id);

					packet.skip(endReadIndex - packet.getReadIndex());
					processPacket(peer, packet, source, false);
				}
			}
			break;
		}
		case DISCONNECT: {
			if(wasSendDirectly) {
				peer->conn->disconnect();
			}
			break;
		}
		default:
			log(3, "unknown opcode %i\n", (int)opcode);
			break;
		}

	}

	void PeerNetwork::sendToNextPeer(PeerId id, Buffer& packet, PeerId except) {
		Peer* next = routingTable.getNext(id, except, false);
		if (next) {
			next->conn->write(packet);
		}
	}

	void PeerNetwork::sendToAllPeers(Buffer& packet, PeerId except) {
		for (auto& peer : routingTable.peers) {
			if (peer && peer->conn) {
				if (peer->id != except) {
					peer->conn->write(packet);
				}
			}
		}
	}

	void PeerNetwork::setState(State newState) {
		state = newState;
		log(3, "state: %s\n", getStateName(state));
	}

	void PeerNetwork::createPacketHandshake(Buffer& packet, PeerId id, uint16_t port, const std::string &address) {
		packet.write(Opcode::HANDSHAKE);
		packet.write(id);
		packet.write(port);
		packet.writeStr(address);
	}

	void PeerNetwork::createPacketLookup(Buffer& packet, PeerId source, PeerId relay, PeerId target) {
		packet.write(Opcode::LOOKUP);
		packet.write(source);
		packet.write(relay);
		packet.write(target);
	}

	void PeerNetwork::createPacketLookupReply(Buffer& packet, PeerId source, PeerId target, uint16_t port, const std::string& address) {
		packet.write(Opcode::LOOKUP_REPLY);
		packet.write(source);
		packet.write(target);
		packet.write(port);
		packet.writeStr(address);
	}

	void PeerNetwork::createPacketRoute(Buffer& packet, PeerId source, PeerId target, uint8_t exact) {
		packet.write(Opcode::ROUTE);
		packet.write(source);
		packet.write(target);
		packet.write(exact);
	}

	void PeerNetwork::log(int level, const char* fmt, ...) {
		if (logCallback && logVerbosity >= level) {
			static char buffer[1024];
			va_list args;
			va_start(args, fmt);
			vsprintf_s(buffer, sizeof(buffer), fmt, args);
			logCallback(buffer);
			va_end(args);
		}
	}

	std::string PeerNetwork::idToStr(PeerId id) {
		std::string str;
		toHex(id, str);
		swapEndianness(str);
		return str.substr(0, 4);
	}

}
