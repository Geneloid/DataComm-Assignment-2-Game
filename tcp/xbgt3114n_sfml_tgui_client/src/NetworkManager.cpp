#include "NetworkManager.h"

size_t NetworkManager::subscribe(PacketHandler handler)
{
	size_t id = nextId++;
	listeners[id] = handler;
	return id;
}

// Removes the listener associated with the ID
void NetworkManager::unsubscribe(size_t id)
{
	listeners.erase(id);
}

bool NetworkManager::connect(const sf::IpAddress& ip, unsigned short port)
{
	if (socket.connect(ip, port) == sf::Socket::Status::Done)
	{
		socket.setBlocking(false);
		return true;
	}
	return false;
}

void NetworkManager::send(sf::Packet& packet)
{
	sendQueue.push(packet);
}

void NetworkManager::update()
{
	//std::cout << "nm update " << std::endl;
	while (!sendQueue.empty())
	{
		// in non-blocking mode, network send may not immediately complete,
		// returning Status::Partial. This means the same packet must
		// continue to be sent out.
		//
		// user would want to send multiple packet, for example, a packet
		// for chat, and another packet for game-related logic. Having
		// a packet queuing system, in non-blocking connection, allows
		// us to gracefully handle Status::Partial for all out packets.
		sf::Socket::Status status = socket.send(sendQueue.front());
		if (status == sf::Socket::Status::Done)
		{
			sendQueue.pop();
		}
		else
		{
			break;
		}
	}

	// SFML 3 handles partial receives internally. When packet is fully
	// received, the status will be Status::Done, and the packet is
	// fully realized.
	sf::Packet incoming;
	sf::Socket::Status status = socket.receive(incoming);

	if (status == sf::Socket::Status::Done)
	{
		handlePacket(incoming);
	}
}

void NetworkManager::handlePacket(sf::Packet& packet)
{
	// PacketHandler signature is void(sf::Packet) which means
	// the 'packet' is passed by value. Each subscriber gets a copy
	// of the packet.
	// This is to ensure that each subscriber can do their own packet
	// unpacking without affecting other subscribers.
	for (auto const& [id, handler] : listeners)
	{
		handler(packet);
	}
}