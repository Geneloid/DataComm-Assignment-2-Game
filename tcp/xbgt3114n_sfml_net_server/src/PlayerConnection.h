#pragma once

#include <SFML/Network.hpp>
#include <string>

class PlayerConnection
{
public:
	PlayerConnection(sf::TcpSocket* socket, int id);
	~PlayerConnection();

	// basic player identity
	void setName(const std::string& name);
	int getId() const;
	const std::string& getName() const;
	sf::TcpSocket* getSocket() const;

	// lobby / room tracking
	void setRoomId(int value);
	int getRoomId() const;
	bool isInLobby() const;

	// room-ready state
	void setReady(bool value);
	bool isReady() const;

	// cosmetic/token selection
	void setToken(int value);
	int getToken() const;

	// higher/lower secret number choice
	void setSecretNumber(int value);
	int getSecretNumber() const;
	bool hasSecretNumberChosen() const;
	void clearSecretNumber();

	// session stats
	void addWin();
	void addLoss();
	int getWins() const;
	int getLosses() const;

private:
	int id;
	std::string name;
	sf::TcpSocket* socket;

	// -1 means player is currently in lobby and not inside a game room
	int roomId = -1;

	bool ready = false;
	int token = 0;

	bool secretNumberChosen = false;
	int secretNumber = -1;

	int wins = 0;
	int losses = 0;
};