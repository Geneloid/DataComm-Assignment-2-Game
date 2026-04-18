#pragma once

#include <SFML/Network.hpp>

class PlayerConnection
{
public:
	PlayerConnection(sf::TcpSocket* socket, int id);

	void setName(const std::string name);

	int getId();
	std::string getName();
	sf::TcpSocket* getSocket();

private:
	int id;
	std::string name;
	sf::TcpSocket* socket;
};
