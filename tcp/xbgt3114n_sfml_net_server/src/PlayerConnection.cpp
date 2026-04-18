#include "PlayerConnection.h"

PlayerConnection::PlayerConnection(sf::TcpSocket* socket, int id)
{
	this->socket = std::move(socket);
	this->id = id;
}

int PlayerConnection::getId()
{
	return id;
}

void PlayerConnection::setName(const std::string name)
{
	this->name = name;
}

std::string PlayerConnection::getName()
{
	return name;
}

sf::TcpSocket* PlayerConnection::getSocket()
{
	return socket;
}