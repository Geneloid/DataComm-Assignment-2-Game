#pragma once
#include <SFML/Network.hpp>
#include <queue>
#include <functional>
#include <map> 

// current changes:
// - PlayerInstance: change id from int to std::string (including in ctor)
// - Server: remove idIncrementor
// - Server: 

struct PendingPacket
{
	sf::Packet packet;
	sf::Clock timer;
	int attempts = 0;
};

class PlayerInstance
{
private:
	std::string id;
	std::string name;
	float lastPacketTime = -1.0f;

	sf::IpAddress ipAddress;
	unsigned short port;

	std::map<std::uint16_t, PendingPacket> pendingReliablePackets;
	std::uint16_t nextSequenceId;

public:

	// calling this function WILL incrrement the nextSequenceId by one after the function
	// call
	std::uint16_t getSequenceIdThenIncrement() { return nextSequenceId++; }

	std::map<std::uint16_t, PendingPacket>& getPendingPackets()
	{
		return pendingReliablePackets;
	}

	void addPendingPacket(sf::Packet& reliablePacket, std::uint16_t sequenceId)
	{
		pendingReliablePackets[sequenceId] = { reliablePacket, sf::Clock(), 0 };
	}

	void receiveAckFor(std::uint16_t sequenceId)
	{
		pendingReliablePackets.erase(sequenceId);
	}

	PlayerInstance(sf::IpAddress ip, unsigned short port, std::string id, std::string name, float lastPacketTime)
		: id(id)
		, name(name)
		, lastPacketTime(lastPacketTime)
		, ipAddress(ip)
		, port(port)
	{
	}

	float getLastPacketTime()
	{
		return lastPacketTime;
	}

	void updateLastPacketTime(float newTime)
	{
		lastPacketTime = newTime;
	}

	sf::IpAddress getIpAddress() const
	{
		return ipAddress;
	}

	unsigned short getPort() const
	{
		return port;
	}
};

class Server
{
private:
	sf::UdpSocket udpSocket;

	std::map<std::string, PlayerInstance> players;

	// C++17 allows us to define static member variables in header using inline
	//inline static int idIncrementor = 0;
	inline static int playerCount = 0;

	float time = 0.0f;

public:

	float getTime() const
	{
		return time;
	}

	void host(unsigned short port);
	void send(sf::Packet& packet);
	void sendTo(sf::Packet& packet, std::string id);
	void sendTo(sf::Packet& packet, PlayerInstance& playerRef);

	void sendReliable(sf::Packet& packet);
	void sendToReliable(sf::Packet& packet, std::string id);
	void sendToReliable(sf::Packet& packet, PlayerInstance& playerRef);

	void update(float deltaTime);
};