#include "PeerRoutingTable.h"
#include "PeerRoutingTable.h"
#include "PeerRoutingTable.h"
#include "PeerRoutingTable.h"
#include "PeerRoutingTable.h"
#include "PeerRoutingTable.h"
//
// Copyright (c) 2023 Julian Hinxlage. All rights reserved.
//

#include "PeerRoutingTable.h"

namespace net {
	
	PeerRoutingTable::PeerRoutingTable() {
		bucketSizeBits = 1;
		localPeer = std::make_shared<Peer>();
	}
	
	Peer* PeerRoutingTable::getNext(const PeerId& id, const PeerId& except, bool includeLocalPeer) {
		PeerId minDist = PeerId(0);
		int peerIndex = -1;

		for (int i = 0; i < peers.size(); i++) {
			if (peers[i]->id != except) {
				PeerId dist = id ^ peers[i]->id;
				if (dist < minDist || peerIndex == -1) {
					minDist = dist;
					peerIndex = i;
				}
			}
		}

		if (includeLocalPeer) {
			PeerId dist = id ^ localPeer->id;
			if (dist < minDist || peerIndex == -1) {
				minDist = dist;
				return localPeer.get();
			}
		}

		if (peerIndex == -1) {
			return nullptr;
		}
		else {
			return peers[peerIndex].get();
		}
	}

	void PeerRoutingTable::add(const Peer& peer) {
		peers.push_back(std::make_shared<Peer>(peer));
	}

	void PeerRoutingTable::remove(const PeerId& id) {
		for (int i = 0; i < peers.size(); i++) {
			if (peers[i]->id == id) {
				peers.erase(peers.begin() + i);
				i--;
			}
		}
	}

	void PeerRoutingTable::remove(Connection* conn) {
		for (int i = 0; i < peers.size(); i++) {
			if (peers[i]->conn == conn) {
				peers.erase(peers.begin() + i);
				i--;
			}
		}
	}

	Peer* PeerRoutingTable::get(const PeerId& id) {
		for (int i = 0; i < peers.size(); i++) {
			if (peers[i]->id == id) {
				return peers[i].get();
			}
		}
		if (localPeer->id == id) {
			return localPeer.get();
		}
		return nullptr;
	}

	Peer* PeerRoutingTable::get(Connection* conn) {
		for (int i = 0; i < peers.size(); i++) {
			if (peers[i]->conn == conn) {
				return peers[i].get();
			}
		}
		return nullptr;
	}

	bool PeerRoutingTable::has(const PeerId& id) {
		for (int i = 0; i < peers.size(); i++) {
			if (peers[i]->id == id) {
				return true;
			}
		}
		return false;
	}

	bool PeerRoutingTable::hasLookupIndexInTable(int index) {
		for (int i = 0; i < peers.size(); i++) {
			if (getLookupIndex(peers[i]->id) == index) {
				return true;
			}
		}
		return false;
	}

	PeerId PeerRoutingTable::getLookupTarget(int index) {
		int bucketSize = 1 << bucketSizeBits;
		index = (sizeof(PeerId) * 8 * bucketSize) - index - 1;

		int bucket = index / bucketSize;
		int bucketIndex = index % bucketSize;
		
		if (bucket - bucketSizeBits < 0) {
			PeerId offset = PeerId(bucketSize + bucketIndex) >> -(bucket - bucketSizeBits);
			return localPeer->id ^ offset;
		}
		else {
			PeerId offset = PeerId(bucketSize + bucketIndex) << (bucket - bucketSizeBits);
			return localPeer->id ^ offset;
		}
	}

	int PeerRoutingTable::getLookupIndex(const PeerId& id) {
		int bucketSize = 1 << bucketSizeBits;
		PeerId offset = localPeer->id ^ id;

		int bucket = 0;
		while ((offset >> 1) >= PeerId(bucketSize)) {
			offset = offset >> 1;
			bucket++;
		}
		int bucketIndex = (int)offset % bucketSize;
		bucket += bucketSizeBits;

		int index = bucket * bucketSize + bucketIndex;
		index = (sizeof(PeerId) * 8 * bucketSize) - index - 1;
		return index;
	}

}
