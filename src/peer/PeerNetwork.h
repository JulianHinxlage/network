//
// Copyright (c) 2023 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "PeerRoutingTable.h"
#include "net/Server.h"
#include <mutex>
#include <set>

namespace net {

	class PeerNetwork {
	public:
		std::function<void(PeerId, Buffer&)> readCallback;
		std::function<void(const char *)> logCallback;
		int logVerbosity = 1;

		enum State {
			DISCONNECTED,
			CONNECTING_TO_ENTRY_NODE,
			SENDING_LOOKUPS,
			LOOKUPS_SEND,
			CONNECTED,
		};

		enum Opcode {
			NONE,
			PING,
			PONG,
			HANDSHAKE,
			HANDSHAKE_REPLY,
			LOOKUP,
			LOOKUP_REPLY,
			ROUTE,
			MESSAGE,
			BROADCAST,
			DISCONNECT,
		};

		PeerNetwork();
		~PeerNetwork();
		void addEntryNode(const std::string& address, uint16_t port);
		bool loadConfigFile(const std::string& file);

		void setLocalPeer(const std::string& localAddress, uint16_t localPort, int maxPortOffset = 0, PeerId id = PeerId(0));
		void connect(bool clientOnly = false);
		void disconnect();
		bool isConnected();
		PeerId getLocalId();
		PeerId getRandomNeighborPeer();
		State getState();

		void send(PeerId id, Buffer& payload, bool exact = true);
		void broadcast(Buffer& payload);
		void broadcastPing();
		std::string idToStr(PeerId id);

	private:
		PeerRoutingTable routingTable;
		std::vector<Peer> entryNodes;
		PeerId localId;
		Server server;

		bool debugFakeLatency = false;

		int lookupCountOnConnect = 64;
		bool clientOnly = false;
		int maxPortOffset = 0;
		Peer* entryNode = nullptr;
		bool wasEntryNodeLookedUp = false;
		State state = DISCONNECTED;
		std::mutex routingTableMutex;
		std::set<PeerId> lookupReplyTargets;
		std::set<PeerId> lookupTargets;
		int loopupRelysRecieved = 0;
		std::set<uint64_t> seenBroadcastNonces;
		std::shared_ptr<std::thread> lookupThread;
		
		bool connectToPeer(const std::string& address, uint16_t port);
		void disconnectFromPeer(Peer *peer);
		void performLookups();
		void onConnect(Connection* conn);
		void onDisconnect(Connection *conn);
		void processPacket(Peer *peer, Buffer &packet, PeerId routingSource, bool wasSendDirectly);

		void sendToNextPeer(PeerId id, Buffer& packet, PeerId except = PeerId(0));
		void sendToAllPeers(Buffer& packet, PeerId except = PeerId(0));
		void setState(State newState);

		void createPacketHandshake(Buffer& packet, PeerId id, uint16_t port, const std::string& address);
		void createPacketLookup(Buffer& packet, PeerId source, PeerId relay, PeerId target);
		void createPacketLookupReply(Buffer& packet, PeerId source, PeerId target, uint16_t port, const std::string& address);
		void createPacketRoute(Buffer& packet, PeerId source, PeerId target, uint8_t exact = 1);

		void log(int level, const char* fmt, ...);
	};

}
