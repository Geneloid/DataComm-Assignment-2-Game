#include <SFML/Network.hpp>
#include <SFML/System.hpp>
#include <list>

struct DelayedPacket
{
	sf::Packet packet;
	sf::IpAddress address;
	unsigned short port;
	sf::Time releaseTime;
};

class LatencySimulationTransport
{
	sf::UdpSocket socket;
	std::list<DelayedPacket> packetQueue;
	sf::Clock clock;

	float baseLatencyMs = 100.0f;
	float jitterMs = 50.0f;

public:

	void setLatencyJitter(float a, float b);
	void setBlocking(bool blocking);

	void update();

	sf::Socket::Status send(sf::Packet& packet, const sf::IpAddress& address, unsigned short port);
	sf::Socket::Status receive(sf::Packet& packet, std::optional<sf::IpAddress>& remoteAddress, unsigned short& remotePort);
};