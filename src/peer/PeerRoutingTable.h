//
// Copyright (c) 2023 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "util/Blob.h"
#include "net/Connection.h"
#include <string>
#include <vector>

namespace net {

	typedef Blob<128> PeerId;

	class Peer {
	public:
		enum State {
			DISCONNECTED,
			PRE_HANDSHAKE,
			CONNECTED,
		};

		PeerId id;
		std::string address;
		uint16_t port;
		Connection* conn;
		State state = DISCONNECTED;
	};

	class PeerRoutingTable {
	public:
		int bucketSizeBits;
		std::vector< std::shared_ptr<Peer>> peers;
		std::shared_ptr<Peer> localPeer;

		PeerRoutingTable();
		Peer* getNext(const PeerId& id, const PeerId& except = PeerId(0), bool includeLocalPeer = false);
		void add(const Peer& peer);
		void remove(const PeerId& id);
		void remove(Connection *conn);
		Peer* get(const PeerId& id);
		Peer* get(Connection* conn);
		bool has(const PeerId& id);
		bool hasLookupIndexInTable(int index);
		PeerId getLookupTarget(int index);
		int getLookupIndex(const PeerId& id);
	};

}
