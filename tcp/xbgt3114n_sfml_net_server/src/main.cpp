#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <iostream>
#include <vector>
#include "PlayerConnection.h"
#include <random>

enum class PacketType : int
{
	HandShake = 0,
	PlayerConnected = 1,
	PlayerDisconnected = 2,
	NameSet = 3,
	MessageSend = 4,
	PlayerListing = 5,
	GameStateUpdate = 6,
	PlayerEndTurn = 7,
	GameStateRequest = 8
};

static std::vector<PlayerConnection> players;

static int idIncrementor = 0;
static int playerCount = 0;

static const int MAX_PLAYER_COUNT = 99;

static int currentPlayerIndex = -1;
static int currentTurnNumber = 0;

void sendPacketAll(sf::Packet& packet, int excludeIndex)
{
	for (unsigned int i = 0; i < players.size(); ++i)
	{
		if (excludeIndex == i)
			continue;

		if (players[i].getSocket()->send(packet) != sf::Socket::Status::Done)
		{
			std::cout << "Error sending packet in sendPacket func" << std::endl;
		}
	}
}

void sendPacketTo(sf::Packet& packet, int index)
{
	if (players[index].getSocket()->send(packet) != sf::Socket::Status::Done)
	{
		std::cout << "Error sending packet in sendPacket func" << std::endl;
	}
}

bool handleClientConnectionRequest(sf::TcpSocket* acceptedSocket)
{
	sf::Packet outPacket;
	outPacket << (int)PacketType::HandShake;

	bool full = playerCount >= MAX_PLAYER_COUNT;
	if (!full)
	{
		std::cout << acceptedSocket->getRemoteAddress().value() << " connected. Assigning id: " << idIncrementor << std::endl;

		outPacket << idIncrementor;

		if (acceptedSocket->send(outPacket) == sf::Socket::Status::Done)
			std::cout << "Sent handshake" << std::endl;

		return true;
	}
	else
	{
		outPacket << -1;

		if (acceptedSocket->send(outPacket) != sf::Socket::Status::Done)
			std::cout << "error sending player index" << std::endl;

		return false;
	}
}

int main()
{
	static sf::TcpListener listener;
	static sf::SocketSelector selector;

	std::random_device rd;
	std::mt19937 gen(rd());

	if (listener.listen(53000) != sf::Socket::Status::Done)
	{
		return -1; // Error
	}

	std::cout << "Server started: " << std::endl;

	selector.add(listener);

	bool isRunning = true;
	while (isRunning)
	{
		if (selector.wait())
		{
			// if this returns true, it means a client wants to connect
			if (selector.isReady(listener))
			{
				sf::TcpSocket* socket = new sf::TcpSocket();
				if (listener.accept(*socket) == sf::Socket::Status::Done)
				{
					if (handleClientConnectionRequest(socket))
					{
						auto& playerSocket = players.emplace_back(PlayerConnection(socket, idIncrementor));
						selector.add(*playerSocket.getSocket());
						idIncrementor++;
						playerCount++;
					}
					else
					{
						socket->disconnect();
						delete socket;
					}
				}
				else delete socket;
			}

			// check if any connecting socket is sending packet to server.
			for (unsigned int i = 0; i < players.size();)
			{
				PlayerConnection& player = players[i];
				sf::TcpSocket* socketPtr = player.getSocket();
				if (selector.isReady(*socketPtr))
				{
					sf::Packet packet;
					sf::Socket::Status status = socketPtr->receive(packet);
					if (status == sf::Socket::Status::Done)
					{
						int type;
						packet >> type;

						if (type == (int)PacketType::NameSet)
						{
							std::string name;
							packet >> name;
							player.setName(name);
							std::cout << "Client " << player.getId() << " name set: " << name << std::endl;

							sf::Packet outPacket;
							outPacket << (int)PacketType::PlayerConnected;
							outPacket << player.getId() << player.getName();

							sendPacketAll(outPacket, -1);
						}
						else if (type == (int)PacketType::MessageSend)
						{
							std::string msg;
							packet >> msg;

							// nicely format the string
							std::string s = player.getName();
							s += ": ";
							s += msg;

							// log it
							std::cout << s << std::endl;

							sf::Packet broadcastPacket;
							broadcastPacket << type;
							broadcastPacket << player.getId();
							broadcastPacket << s;

							sendPacketAll(broadcastPacket, -1);
						}
						//else if (type == (int)PacketType::PlayerListing)
						//{
						//	sf::Packet outPacket;
						//	outPacket << (int)PacketType::PlayerListing;
						//	outPacket << playerCount;
						//	for (auto& pl : players)
						//	{
						//		outPacket << pl.getId();
						//		outPacket << pl.getName();
						//	}
						//	sendPacketTo(outPacket, i);
						//}
						else if (type == (int)PacketType::GameStateRequest) // A player sent this request to get current game state
						{
							// game just started, so choose a random player
							if (currentPlayerIndex < 0)
							{
								std::uniform_int_distribution<> distrib(0, playerCount - 1);
								currentPlayerIndex = distrib(gen);
							}

							auto& currentPlayer = players[currentPlayerIndex];
							std::string s = currentPlayer.getName();
							s += "'s turn!";

							sf::Packet outPacket;
							outPacket << (int)PacketType::GameStateUpdate
								<< currentPlayer.getId()
								<< s
								<< currentTurnNumber;

							// Send only to the requester
							sendPacketTo(outPacket, i);
						}
						else if (type == (int)PacketType::PlayerEndTurn) // Player send this request to end their turn, and server needs to select next player
						{
							// Only proceed if the requester is the one that has their turn
							if (players[currentPlayerIndex].getId() == player.getId())
							{
								std::cout << player.getName() << " ended turn " << currentTurnNumber;

								currentTurnNumber++;

								std::uniform_int_distribution<> distrib(0, playerCount - 1);
								currentPlayerIndex = distrib(gen);
								auto& currentPlayer = players[currentPlayerIndex];
								std::string s = currentPlayer.getName() + "'s turn!";

								sf::Packet outPacket;
								outPacket << (int)PacketType::GameStateUpdate
									<< currentPlayer.getId()
									<< s
									<< currentTurnNumber;

								std::cout << ", now it is turn " << currentTurnNumber << " given to " << currentPlayer.getName() << std::endl;
								sendPacketAll(outPacket, -1);
							}
							else
							{
								std::cout << player.getId() << "tried to end their turn when it is not theirs!" << std::endl;
							}
						}
					}
					else if (status == sf::Socket::Status::Disconnected)
					{
						sf::Packet packet;
						packet << (int)PacketType::PlayerDisconnected;

						std::string s = player.getName();
						s += " disconnected. Bye bye!";

						packet << s;

						sendPacketAll(packet, i);

						int cachedCurrentPlayerId = players[currentPlayerIndex].getId();
						int cachedPlayerId = player.getId();
						std::cout << player.getName() << " disconnected." << std::endl;
						selector.remove(*socketPtr);
						players.erase(players.begin() + i);
						playerCount--;

						// current turn is the disconnected player,
						// so the game needs to look for a different player
						// to make sure game state is valid
						if (cachedCurrentPlayerId == cachedPlayerId)
						{
							if (players.size() > 0)
							{
								// note that this part is copy pasted from:
								//		else if (type == (int)PacketType::PlayerEndTurn)
								std::cout << player.getName() << " ended turn " << currentTurnNumber;

								currentTurnNumber++;

								std::uniform_int_distribution<> distrib(0, playerCount - 1);
								currentPlayerIndex = distrib(gen);
								auto& currentPlayer = players[currentPlayerIndex];
								std::string s = currentPlayer.getName() + "'s turn!";

								sf::Packet outPacket;
								outPacket << (int)PacketType::GameStateUpdate
									<< currentPlayer.getId()
									<< s
									<< currentTurnNumber;

								std::cout << ", now it is turn " << currentTurnNumber << " given to " << currentPlayer.getName() << std::endl;
								sendPacketAll(outPacket, -1);
							}
							else
							{
								// if no player, reset game state to init
								currentPlayerIndex = -1;
								currentTurnNumber = 0;
							}
						}

						continue;
					}
				}
				i++;
			}
		}
	}
}