#pragma once
#include <SFML/Network.hpp>
#include <queue>
#include <functional>
#include <map>
#include <string>
#include <vector>

// aliasing the std::function
using PacketHandler = std::function<void(sf::Packet)>;

class NetworkManager
{
private:
	sf::TcpSocket socket;
	std::queue<sf::Packet> sendQueue;

	std::map<size_t, PacketHandler> listeners;
	size_t nextId = 0;

	// client-side session data
	// only help current client UI understand who "I" am and what I entered
	int localPlayerId = -1;
	std::string localPlayerName;

	int mySecretNumber = -1;
	std::vector<std::string> myGuessHistory;

public:

	size_t subscribe(PacketHandler handler);
	void unsubscribe(size_t id);

	bool connect(const sf::IpAddress& ip, unsigned short port);
	void send(sf::Packet& packet);
	void update();
	void handlePacket(sf::Packet& packet);

	// local player identity helpers
	void setLocalPlayerInfo(int id, const std::string& name);
	int getLocalPlayerId() const;
	const std::string& getLocalPlayerName() const;

	// personal gameplay helpers
	void setMySecretNumber(int value);
	int getMySecretNumber() const;
	void clearMySecretNumber();

	void addMyGuessHistory(const std::string& entry);
	const std::vector<std::string>& getMyGuessHistory() const;
	void clearMyGuessHistory();
};