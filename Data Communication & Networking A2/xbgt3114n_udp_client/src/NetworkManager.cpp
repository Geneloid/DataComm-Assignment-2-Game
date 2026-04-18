#include "NetworkManager.h"
#include "PacketTypes.h"
#include "PacketHeader.h"
#include <iostream>

static bool connectPending = false;
static float connectTimeOut = 0.0f;

size_t NetworkManager::subscribe(PacketHandler handler)
{
	size_t id = nextListenerId++;
	listeners[id] = handler;
	return id;
}

// Removes the listener associated with the ID
void NetworkManager::unsubscribe(size_t id)
{
	listeners.erase(id);
}

bool NetworkManager::connect(std::string name, const sf::IpAddress& ip, unsigned short port)
{
	transport.setBlocking(true);

	PacketHeader h = { PacketType::Unreliable, 0 };
	sf::Packet packet;
	packet << h; // technically should be reliable
	packet << (int)RequestPacketType::ConnectRequest;
	packet << name;
	if (transport.send(packet, ip, port) == sf::Socket::Status::Done)
	{
		connectPending = true;
		connectTimeOut = time + 5.0f;

		// in UDP, sending out always succeed as long as the packet
		// size is less than sf::UdpSocket::MaxDatagramSize

		// we wouldn't know whether server received our connection yet!
		std::cout << "Connecting to " << ip << ":" << port << "..." << std::endl;
	}

	// since we sent out request, server should send us back response

	sf::Packet incoming;
	std::optional<sf::IpAddress> sender;
	unsigned short               senderPort = 0;
	sf::Socket::Status status = transport.receive(incoming, sender, senderPort);

	// PONDER:
	// What if bad actor sent us something in between our request>response sequence???
	// Need to check if 'sender' and 'senderPort' matches 'ip' and 'port'
	// if not, restart receiving
	// until when? make a timeout!
	//
	// In this example, we assume no such thing as bad stuff. Dream world.

	sf::Packet copyIncoming = incoming;
	PacketHeader inHeader;
	int type;
	copyIncoming >> inHeader >> type;

	// We assume server response is Unreliable

	if (type == (int)ResponsePacketType::ConnectAccepted)
	{
		transport.setBlocking(false);
		connectPending = false;
		connectedServerIp = ip;
		connectedServerPort = port;
		std::cout << "Connected to " << ip << ":" << port << std::endl;

		handlePacket(incoming);

		sf::Packet timePacket;
		timePacket << 0;
		timePacket << (int)RequestPacketType::TimeSyncRequest;
		timePacket << time;
		send(timePacket);

		return true;
	}

	return false;
}

// 0: not connected
// 1: connected
// 2: pending
int NetworkManager::getConnectStatus()
{
	if (connectPending) return 2; // pending
	else return connectedServerIp.has_value() ? 1 : 0;
}

void NetworkManager::send(sf::Packet& packet)
{
	if (!connectedServerIp.has_value())
		return;

	PacketHeader h = { PacketType::Unreliable, 0 };
	sf::Packet wrappedPacket;
	wrappedPacket << h;
	wrappedPacket.append(packet.getData(), packet.getDataSize());

	// only send out when connected to server.
	sf::Socket::Status status = transport.send(packet, connectedServerIp.value(), connectedServerPort);

	if (status == sf::Socket::Status::Done)
	{
		std::cout << "Sent!" << std::endl;
	}
}
void NetworkManager::sendReliable(sf::Packet& packet)
{
	if (!connectedServerIp.has_value())
		return;

	// TODO: Implement reliable on top of UDP
	//       Server needs to respond that it had received
}

void NetworkManager::update(float deltaTime)
{
	// Only available in LatencySimulationTransport
	// Comment out if reverting to sf::UdpSocket
	transport.update();

	time += deltaTime;

	if (connectPending && time > connectTimeOut)
	{
		connectPending = false;
		std::cout << "Failed to connect" << std::endl;
	}

	// if we are not connected to server, don't proceed.
	if (!connectedServerIp.has_value())
		return;

	sf::Packet incoming;
	std::optional<sf::IpAddress> sender;
	unsigned short               senderPort = 0;
	sf::Socket::Status status = transport.receive(incoming, sender, senderPort);

	// partial send/receive in UDP connection
	if (status == sf::Socket::Status::Done)
	{
		// only handle received packet if we got it from the connected server
		if (sender.value() == connectedServerIp.value() && connectedServerPort == senderPort)
		{
			handlePacket(incoming);
		}
		else
		{
			// something else is sending us data. Do nothing
		}
	}

	// TODO: Reliable resending after some period have passed.
	//       Each reliable packet has timer

	// TODO: Periodic time synchronization
	// if time sync fails several times in a row, then assume server is dead

	if (time > nextSyncTime)
	{
		if (syncPending)
		{

		}
		else
		{

		}

		syncPending = true;
		nextSyncTime = time + 5.0f;

		sf::Packet timePacket;
		timePacket << (int)RequestPacketType::TimeSyncRequest;
		timePacket << time;
		send(timePacket);
	}
}

void NetworkManager::handlePacket(sf::Packet& packet)
{
	// Using the packet directly, consume the header
	// If header type is Reliable:
	//		check if the header sequence matches what we want (nextSequenceId)
	//			if so:
	//				increment nextSequenceId by 1.
	//				Continue downwards
	//			otherwise:
	//				stop and return

	// If header is Unreliable, OR Reliable and "Continue downwards"
	//		Make a new packet containing the data from the packet.
	//		distribute to listeners.
	//

	bool continueOn = true;
	PacketHeader h;
	packet >> h;

	if (h.type == PacketType::Ack)
	{
		// TODO: Manage pending packets
	}

	else if (h.type == PacketType::Reliable)
	{
		std::cout << "Received sequenceId: " << h.sequenceId << " | Expecting: " << nextSequenceId;
		if (h.sequenceId != nextSequenceId)
		{
			std::cout << " [Reject]" << std::endl;
			continueOn = false;
		}
		else
		{
			std::cout << " [Accept]" << std::endl;
			nextSequenceId++;
			PacketHeader ackHeader = { PacketType::Ack, h.sequenceId };
			sf::Packet ackPacket;
			ackPacket << ackHeader;
			transport.send(ackPacket, connectedServerIp.value(), connectedServerPort);
		}
	}

	if (!continueOn) return;

	// Note that packet has stripped header
	//
	// packet
	// - Header (EXTRACTED)
	// - ResponseType
	// - Data
	// 

	// Make a copy so we can extract ResponseType
	sf::Packet copy = sf::Packet(packet);

	int type;
	copy >> type;

	// if the response is time synchronization, handle it internally and not send out
	if (type == (int)ResponsePacketType::TimeSyncResponse)
	{
		float currentTime = time;

		float serverTime, sentTime;

		rtt = currentTime - sentTime;
		time = serverTime + rtt * 0.5f;

		syncPending = false;
		nextSyncTime = time + 5.0f;
	}
	else
	{
		// We can send packet directly
		// This will make a copy of packet where the read cursor is also copied
		//
		//                [-Header-][-ResponseType-][-Data-]
		// packet
		// copy
		//
		// Or we could recreate a new packet, where we only append [TYPE] [ DATA ]
		//
		sf::Packet trimmedPacket;
		if (packet.getDataSize() > packet.getReadPosition())
		{
			std::size_t remainingSize = packet.getDataSize() - packet.getReadPosition();
			trimmedPacket.append((const char*)packet.getData() + packet.getReadPosition(), remainingSize);
		}

		for (auto const& [id, handler] : listeners)
		{
			handler(trimmedPacket);
		}
	}
}