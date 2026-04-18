#include "PlayerConnection.h"

PlayerConnection::PlayerConnection(sf::TcpSocket* socket, int id)
{
	this->socket = socket;
	this->id = id;
}

PlayerConnection::~PlayerConnection()
{
	if (socket != nullptr)
	{
		socket->disconnect();
		delete socket;
		socket = nullptr;
	}
}

void PlayerConnection::setName(const std::string& name)
{
	this->name = name;
}

int PlayerConnection::getId() const
{
	return id;
}

const std::string& PlayerConnection::getName() const
{
	return name;
}

sf::TcpSocket* PlayerConnection::getSocket() const
{
	return socket;
}

void PlayerConnection::setRoomId(int value)
{
	roomId = value;
}

int PlayerConnection::getRoomId() const
{
	return roomId;
}

bool PlayerConnection::isInLobby() const
{
	return roomId == -1;
}

void PlayerConnection::setReady(bool value)
{
	ready = value;
}

bool PlayerConnection::isReady() const
{
	return ready;
}

void PlayerConnection::setToken(int value)
{
	token = value;
}

int PlayerConnection::getToken() const
{
	return token;
}

void PlayerConnection::setSecretNumber(int value)
{
	secretNumber = value;
	secretNumberChosen = true;
}

int PlayerConnection::getSecretNumber() const
{
	return secretNumber;
}

bool PlayerConnection::hasSecretNumberChosen() const
{
	return secretNumberChosen;
}

void PlayerConnection::clearSecretNumber()
{
	secretNumberChosen = false;
	secretNumber = -1;
}

void PlayerConnection::addWin()
{
	++wins;
}

void PlayerConnection::addLoss()
{
	++losses;
}

int PlayerConnection::getWins() const
{
	return wins;
}

int PlayerConnection::getLosses() const
{
	return losses;
}
