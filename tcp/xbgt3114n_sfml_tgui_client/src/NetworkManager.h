#pragma once
#include <SFML/Network.hpp>
#include <queue>
#include <functional>
#include <map>

// aliasing the std::function
using PacketHandler = std::function<void(sf::Packet)>;

class NetworkManager
{
private:
	sf::TcpSocket socket;
	std::queue<sf::Packet> sendQueue;

	std::map<size_t, PacketHandler> listeners;
	size_t nextId = 0;

public:

	size_t subscribe(PacketHandler handler);
	void unsubscribe(size_t id);

	bool connect(const sf::IpAddress& ip, unsigned short port);
	void send(sf::Packet& packet);
	void update();
	void handlePacket(sf::Packet& packet);
};