#include "LatencySimulationTransport.h"
#include <random>

void LatencySimulationTransport::setLatencyJitter(float baseLatencyMs, float jitterMs)
{
	if (baseLatencyMs < 0.0f) baseLatencyMs = 0.0f;
	if (jitterMs < 0.0f) jitterMs = 0.0f;

	this->baseLatencyMs = baseLatencyMs;
	this->jitterMs = jitterMs;
}

void LatencySimulationTransport::setBlocking(bool blocking)
{
	socket.setBlocking(blocking);
}

void LatencySimulationTransport::update()
{
	if (socket.isBlocking())
		return;

	sf::Packet packet;
	std::optional<sf::IpAddress> sender;
	unsigned short               senderPort = 0;

	// Non-blocking receive
	while (socket.receive(packet, sender, senderPort) == sf::Socket::Status::Done)
	{
		// Calculate delay: base + random jitter
		float randomDelay = 0.0f;

		if (jitterMs > 0)
			randomDelay = static_cast<float>(rand() % (int)jitterMs);
		sf::Time delay = sf::milliseconds(baseLatencyMs + randomDelay);

		packetQueue.push_back({ packet, sender.value(), senderPort, clock.getElapsedTime() + delay });
	}
}

sf::Socket::Status LatencySimulationTransport::send(sf::Packet& packet, const sf::IpAddress& address, unsigned short port)
{
	// Option to simulate packet loss here
	return socket.send(packet, address, port);
}

sf::Socket::Status LatencySimulationTransport::receive(sf::Packet& packet, std::optional<sf::IpAddress>& remoteAddress, unsigned short& remotePort)
{
	if (socket.isBlocking())
	{
		return socket.receive(packet, remoteAddress, remotePort);
	}
	else
	{
		auto it = packetQueue.begin();
		while (it != packetQueue.end())
		{
			if (clock.getElapsedTime() >= it->releaseTime)
			{
				packet = it->packet;
				remoteAddress = it->address;
				remotePort = it->port;
				packetQueue.erase(it);
				return sf::Socket::Status::Done;
			}
			++it;
		}
		return sf::Socket::Status::NotReady;
	}
}