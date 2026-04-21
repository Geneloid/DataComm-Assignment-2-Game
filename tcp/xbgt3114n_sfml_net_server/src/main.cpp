#include <SFML/Network.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <random>
#include <algorithm>
#include <memory>

#include "PlayerConnection.h"
#include "GameRoom.h"
#include "PacketType.h"

/// SUMMARY
/// PacketType used between client and server
/// both client and server need agree on these values
/// if client send a packet with one of these types
/// the server will know how to interpret the remaining data inside that packet

// I moved my PacketTypes class to shared file PacketType.h so both client and server can include and use the same enum values

// ============================
// GLOBAL SERVER DATA
// ============================
//
// players:
//   hold all currently connected players
//
// rooms:
//   hold all currently existing game rooms
//
// idIncrementor:
//   give each connecting player a unique server-side ID
//
// roomIdIncrementor:
//   gives each created room a unique room ID
//
// playerCount:
//   current number of connected players
//
// SECRET_MIN / SECRET_MAX:
//   range for the "Higher or Lower" secret number game
static std::vector<std::unique_ptr<PlayerConnection>> players;
static std::vector<GameRoom> rooms;

static int idIncrementor = 0;
static int roomIdIncrementor = 1;
static int playerCount = 0;

static const int MAX_PLAYER_COUNT = 99;
static const int SECRET_MIN = 1;
static const int SECRET_MAX = 100;

// ============================
// PLAYER / ROOM LOOKUP HELPERS
// ============================
//
// - players vector stores PlayerConnection objects
// - rooms vector stores GameRoom objects
// - GameRoom only stores player IDs not direct player objects
//
// means if the room wants data about a player
// will look up that player ID in the players container

int findPlayerIndexById(int playerId)
{
	for (int i = 0; i < (int)players.size(); ++i)
	{
		if (players[i]->getId() == playerId)
			return i;
	}
	return -1;
}

PlayerConnection* getPlayerById(int playerId)
{
	int index = findPlayerIndexById(playerId);
	if (index < 0)
		return nullptr;
	return players[index].get();
}

GameRoom* getRoomById(int roomId)
{
	for (auto& room : rooms)
	{
		if (room.getRoomId() == roomId)
			return &room;
	}
	return nullptr;
}

int findRoomIndexById(int roomId)
{
	for (int i = 0; i < (int)rooms.size(); ++i)
	{
		if (rooms[i].getRoomId() == roomId)
			return i;
	}
	return -1;
}

// ============================
// PACKET SENDING
// ============================
//
// network sending utilities
//
// sendPacketToPlayer:
//   sends to one specific PlayerConnection
//
// sendPacketToPlayerId:
//   same as above justplayer first by ID
//
// sendPacketAll:
//   send to every connected player
//
// sendPacketToLobby
//   send only to players that are currently in the lobby
//
// sendPacketToRoom
//   send only to players inside one specific room
//
// for different group "audiences" for different events
//
// - global audience (everyone)
// - lobby-only audience
// - room-only audience
//

void sendPacketToPlayer(sf::Packet& packet, PlayerConnection* player)
{
	if (player == nullptr)
		return;

	if (player->getSocket()->send(packet) != sf::Socket::Status::Done)
	{
		std::cout << "Error sending packet to player " << player->getId() << std::endl;
	}
}

void sendPacketToPlayerId(sf::Packet& packet, int playerId)
{
	sendPacketToPlayer(packet, getPlayerById(playerId));
}

void sendPacketAll(sf::Packet& packet, int excludePlayerId = -1)
{
	for (auto& player : players)
	{
		if (player->getId() == excludePlayerId)
			continue;

		if (player->getSocket()->send(packet) != sf::Socket::Status::Done)
		{
			std::cout << "Error sending packet in sendPacketAll()" << std::endl;
		}
	}
}

void sendPacketToLobby(sf::Packet& packet, int excludePlayerId = -1)
{
	for (auto& player : players)
	{
		// players inside a room still able to access lobby chat
		// lobby-chat packet are sent to all connected players
		//
		// client side can decide to show this in a lobby tab
		// or separate panel, or switched chat window
		if (player->getId() == excludePlayerId)
			continue;

		if (player->getSocket()->send(packet) != sf::Socket::Status::Done)
		{
			std::cout << "Error sending lobby packet" << std::endl;
		}
	}
}

void sendPacketToRoom(sf::Packet& packet, GameRoom* room, int excludePlayerId = -1)
{
	if (room == nullptr)
		return;

	// room only stores player ID
	// for each player ID in the room, send the packet to that player
	for (int playerId : room->getPlayerIds())
	{
		if (playerId == excludePlayerId)
			continue;

		sendPacketToPlayerId(packet, playerId);
	}
}

// ============================
//FEEDBACK / SYSTEM MESSAGES
// ============================
//
// sendErrorMessage
//   sends one error packet back to one player
// useful for invalid actions like
//   - trying to guess when it is not your turn
//   - trying to join a full room
//   - trying to change token while READY
//
// broadcastLobbySystemMessage:
//   server-generated message that appear in lobby chat
//
// broadcastRoomSystemMessage:
//   server-generated message that appear in room chat
//
// for server event messaging like player joined / disconnected / won

void sendErrorMessage(PlayerConnection* player, const std::string& message)
{
	sf::Packet outPacket;
	outPacket << (int)PacketType::ErrorMessage;
	outPacket << message;
	sendPacketToPlayer(outPacket, player);
}

void broadcastLobbySystemMessage(const std::string& message)
{
	std::cout << "[Lobby] " << message << std::endl;

	sf::Packet outPacket;
	outPacket << (int)PacketType::LobbyMessageReceive;
	outPacket << -1; // -1 means came from server/system instead of a player
	outPacket << message;
	sendPacketToLobby(outPacket);
}

void broadcastRoomSystemMessage(GameRoom* room, const std::string& message)
{
	if (room == nullptr)
		return;

	std::cout << "[Room " << room->getRoomId() << "] " << message << std::endl;

	sf::Packet outPacket;
	outPacket << (int)PacketType::RoomMessageReceive;
	outPacket << -1; // -1 means came from server/system instead of a player
	outPacket << message;
	sendPacketToRoom(outPacket, room);
}

// ============================
// ROOM LIST / ROOM PLAYER UI SYNCHRONIZATION
// ============================
//
// HOPE THAT THESE SUPPORT the server can keep client UI updated
//
// sendRoomListUpdateToAll:
//   sends the current list of all rooms to all players
//   allows clients to show available rooms in the lobby
//
// sendRoomPlayerListUpdate
//   sends the current player listing for one room to players in that room
//   so allows room UI to show
//   - which players are in the room
//   - their ready states
//   - their token selection
//   - their win/loss counts
//
// sendSessionStatsUpdate:
//   sends only session win/loss counts for players in a room
//

void sendRoomListUpdateToAll()
{
	for (auto& targetPlayer : players)
	{
		sf::Packet outPacket;
		outPacket << (int)PacketType::RoomListUpdate;
		outPacket << (int)rooms.size();

		for (auto& room : rooms)
		{
			outPacket << room.getRoomId();
			outPacket << room.getRoomName();
			outPacket << (int)room.getPlayerIds().size();
			outPacket << (int)room.getState();
		}

		sendPacketToPlayer(outPacket, targetPlayer.get());
	}
}

void sendRoomPlayerListUpdate(GameRoom* room)
{
	if (room == nullptr)
		return;

	sf::Packet outPacket;
	outPacket << (int)PacketType::RoomPlayerListUpdate;
	outPacket << room->getRoomId();
	outPacket << (int)room->getPlayerIds().size();

	for (int playerId : room->getPlayerIds())
	{
		PlayerConnection* player = getPlayerById(playerId);
		if (player == nullptr)
			continue;

		// packet sends all the UI-relevant room player data
		outPacket << player->getId();
		outPacket << player->getName();
		outPacket << player->isReady();
		outPacket << player->getToken();
		outPacket << player->getWins();
		outPacket << player->getLosses();
	}

	sendPacketToRoom(outPacket, room);
}

void sendSessionStatsUpdate(GameRoom* room)
{
	if (room == nullptr)
		return;

	sf::Packet outPacket;
	outPacket << (int)PacketType::SessionStatsUpdate;
	outPacket << room->getRoomId();
	outPacket << (int)room->getPlayerIds().size();

	for (int playerId : room->getPlayerIds())
	{
		PlayerConnection* player = getPlayerById(playerId);
		if (player == nullptr)
			continue;

		outPacket << player->getId();
		outPacket << player->getWins();
		outPacket << player->getLosses();
	}

	sendPacketToRoom(outPacket, room);
}

// ============================
// GAME RULE CHECK
// ============================
//
// allPlayersReadyAndChosenNumbers:
//   this checks whether a room is allowed to start the match
//
// Higher/Lower game, a valid start probably need
// - exactly 2 players
// - both are READY
// - both already submitted their secret number
//
// getOpponentPlayerId:
//   since this is a 2-player game, the opponent is simply
//   the other player in the room

bool allPlayersReadyAndChosenNumbers(GameRoom* room)
{
	if (room == nullptr)
		return false;

	if (room->getPlayerIds().size() != 2)
		return false;

	for (int playerId : room->getPlayerIds())
	{
		PlayerConnection* player = getPlayerById(playerId);
		if (player == nullptr)
			return false;

		if (!player->isReady())
			return false;

		if (!player->hasSecretNumberChosen())
			return false;
	}

	return true;
}

int getOpponentPlayerId(GameRoom* room, int playerId)
{
	if (room == nullptr)
		return -1;

	for (int id : room->getPlayerIds())
	{
		if (id != playerId)
			return id;
	}

	return -1;
}

// ============================
// MATCH START LOGIC (game room)
// ============================
//
// startMatch:
//   actually transitions a room into active gameplay
//
// state relationship
// - before this, room is in waiting/countdown style states
// - after this, room is in InGame state
//
// - resets old match data
// - randomly chooses first turn starter
// - notifies clients that game started
// - notifies clients whose turn it is
//
// tryStartMatchIfReady:
//   checks if room hit start conditions
//   if yes sends countdown message and begin
//
// note:
// no real timer system yet but can be added later hehe

void startMatch(GameRoom* room, std::mt19937& gen)
{
	if (room == nullptr)
		return;

	room->setState(RoomState::InGame);
	room->resetMatchState();
	room->setState(RoomState::InGame);

	// randomly choose the starting player
	std::uniform_int_distribution<> distrib(0, (int)room->getPlayerIds().size() - 1);
	int starterId = room->getPlayerIds()[distrib(gen)];
	room->setCurrentTurnPlayerId(starterId);

	// notify both clients that gameplay has started
	sf::Packet startPacket;
	startPacket << (int)PacketType::GameStart;
	startPacket << room->getRoomId();
	sendPacketToRoom(startPacket, room);

	PlayerConnection* starter = getPlayerById(starterId);
	std::string turnText = starter != nullptr ? starter->getName() + "'s turn!" : "Turn started";

	// TurnUpdate tells the clients
	// - which room this belongs to
	// - whose turn it is
	// - current turn number
	// - display text for UI
	sf::Packet turnPacket;
	turnPacket << (int)PacketType::TurnUpdate;
	turnPacket << room->getRoomId();
	turnPacket << starterId;
	turnPacket << room->getTurnNumber();
	turnPacket << turnText;
	sendPacketToRoom(turnPacket, room);

	broadcastRoomSystemMessage(room, "All players are ready. Match started.");
}

void tryStartMatchIfReady(GameRoom* room, std::mt19937& gen)
{
	if (room == nullptr)
		return;

	if (!allPlayersReadyAndChosenNumbers(room))
		return;

	// avoid restarting countdown if already counting down
	if (room->getState() == RoomState::CountdownStarting && room->isCountdownActive())
		return;

	room->setState(RoomState::CountdownStarting);
	room->startCountdown(3);

	sf::Packet countdownPacket;
	countdownPacket << (int)PacketType::GameStartCountdown;
	countdownPacket << room->getRoomId();
	countdownPacket << 3;
	sendPacketToRoom(countdownPacket, room);

	broadcastRoomSystemMessage(room, "All players ready. Starting in 3 seconds...");
}

// countdown update helper for validation
void updateRoomCountdowns(std::mt19937& gen)
{
	for (auto& room : rooms)
	{
		if (room.getState() == RoomState::CountdownStarting && room.isCountdownActive())
		{
			// if any player unready / leave / clears number during countdown
			// cancel the countdown safely
			if (!allPlayersReadyAndChosenNumbers(&room))
			{
				room.clearCountdown();
				room.setState(RoomState::WaitingReady);
				broadcastRoomSystemMessage(&room, "Countdown cancelled.");
				continue;
			}

			if (room.hasCountdownFinished())
			{
				room.clearCountdown();
				startMatch(&room, gen);
			}
		}
	}
}

// ============================
// HANDSHAKE / CONNECTION ACCEPTANCE
// ============================
//
// runs before a player is inserted into the live player list
// so can accept or reject incoming client based on server capacity and assign client ID
//client waits for this handshake first before sending its name

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
			std::cout << "Error sending player index" << std::endl;

		return false;
	}
}

// ============================
// ROOM MANAGEMENT (lobby and room state transitions)
// ============================
//
// handleCreateRoom:
//   a lobby player creates a room
//   that player instantly become part of the room
//
// handleJoinRoom:
//   a lobby player joins an existing room if it exists and is not full
//
// handleLeaveRoom:
//   a room player leaves the room and returns to lobby.
//
// Data relationship to notice:
// - player's roomId changes
// - room's player list changes
// - room list packet may need to be updated
// - room player list packet may need to be updated

void handleCreateRoom(PlayerConnection* player, const std::string& roomName)
{
	if (player == nullptr)
		return;

	if (!player->isInLobby())
	{
		sendErrorMessage(player, "You must be in the lobby to create a room.");
		return;
	}

	GameRoom newRoom(roomIdIncrementor++, roomName, player->getId());
	rooms.push_back(newRoom);

	player->setRoomId(newRoom.getRoomId());
	player->setReady(false);
	player->clearSecretNumber();

	std::string msg = player->getName() + " created room " + roomName;
	broadcastLobbySystemMessage(msg);

	GameRoom* createdRoom = getRoomById(newRoom.getRoomId());
	if (createdRoom != nullptr)
	{
		createdRoom->setState(RoomState::WaitingReady);
		sendRoomPlayerListUpdate(createdRoom);
	}

	sendRoomListUpdateToAll();

	sf::Packet response;
	response << (int)PacketType::CreateRoomResponse;
	response << true;
	response << newRoom.getRoomId();
	response << roomName;
	sendPacketToPlayer(response, player);
}

void handleJoinRoom(PlayerConnection* player, int roomId)
{
	if (player == nullptr)
		return;

	if (!player->isInLobby())
	{
		sendErrorMessage(player, "You must be in the lobby to join a room.");
		return;
	}

	GameRoom* room = getRoomById(roomId);
	if (room == nullptr)
	{
		sendErrorMessage(player, "Room does not exist.");
		return;
	}

	if (room->isFull())
	{
		sendErrorMessage(player, "Room is already full.");
		return;
	}

	if (!room->addPlayer(player->getId()))
	{
		sendErrorMessage(player, "Unable to join room.");
		return;
	}

	player->setRoomId(roomId);
	player->setReady(false);
	player->clearSecretNumber();

	if (room->getPlayerIds().size() >= 2)
		room->setState(RoomState::WaitingReady);

	std::string msg = player->getName() + " joined room " + room->getRoomName();
	broadcastLobbySystemMessage(msg);
	broadcastRoomSystemMessage(room, player->getName() + " joined the room.");

	sendRoomPlayerListUpdate(room);
	sendRoomListUpdateToAll();

	sf::Packet response;
	response << (int)PacketType::JoinRoomResponse;
	response << true;
	response << roomId;
	sendPacketToPlayer(response, player);
}

void handleLeaveRoom(PlayerConnection* player)
{
	if (player == nullptr)
		return;

	int roomId = player->getRoomId();
	if (roomId < 0)
		return;

	GameRoom* room = getRoomById(roomId);
	if (room == nullptr)
	{
		// if room somehow no longer exists, at least reset player state
		player->setRoomId(-1);
		player->setReady(false);
		player->clearSecretNumber();
		return;
	}

	std::string playerName = player->getName();
	room->removePlayer(player->getId());

	player->setRoomId(-1);
	player->setReady(false);
	player->clearSecretNumber();

	if (!room->isEmpty())
	{
		broadcastRoomSystemMessage(room, playerName + " left the room.");
		sendRoomPlayerListUpdate(room);
	}

	int roomIndex = findRoomIndexById(roomId);
	if (roomIndex >= 0 && rooms[roomIndex].isEmpty())
	{
		std::cout << "Room " << rooms[roomIndex].getRoomName() << " removed because it became empty." << std::endl;
		rooms.erase(rooms.begin() + roomIndex);
	}

	sendRoomListUpdateToAll();
}

// ======================================
// MAIN GAMEPLAY GUESSING LOGIC (turn based gameplay)
// =======================================
//
//
// handleGuess:
//   validates and processes a player's guess
//   server is authoritative
//   a guess is correct or whether the turn should switch
//
// sequence for the function
// 1. validate room and game state
// 2. validate that it is actually this player's turn
// 3. validate guess range
// 4. compare guess against opponent's secret number
// 5. broadcast result (Higher / Lower / Correct)
//  if correct
//      - end match
//      - update win/loss stats
//      - notify room + lobby
//      - reset room to waiting state for next round
//  if incorrect
//      - increment turn counter
//      - switch turn to opponent
//      - notify room whose turn is next
//

void handleGuess(PlayerConnection* player, GameRoom* room, int guessedNumber)
{
	if (player == nullptr || room == nullptr)
		return;

	if (room->getState() != RoomState::InGame)
	{
		sendErrorMessage(player, "Game is not currently in progress.");
		return;
	}

	if (room->getCurrentTurnPlayerId() != player->getId())
	{
		sendErrorMessage(player, "It is not your turn.");
		return;
	}

	if (guessedNumber < SECRET_MIN || guessedNumber > SECRET_MAX)
	{
		sendErrorMessage(player, "Guess must be between 1 and 100.");
		return;
	}

	int opponentId = getOpponentPlayerId(room, player->getId());
	PlayerConnection* opponent = getPlayerById(opponentId);

	if (opponent == nullptr || !opponent->hasSecretNumberChosen())
	{
		sendErrorMessage(player, "Opponent secret number not available.");
		return;
	}

	int opponentSecret = opponent->getSecretNumber();

	std::cout << "[Guess Debug] "
		<< player->getName() << " guessed " << guessedNumber
		<< " against " << opponent->getName()
		<< "'s secret number " << opponentSecret << std::endl;

	std::string resultText;
	bool correct = false;

	if (guessedNumber < opponentSecret)
		resultText = "Higher";
	else if (guessedNumber > opponentSecret)
		resultText = "Lower";
	else
	{
		resultText = "Correct";
		correct = true;
	}

	// GuessResult is sent to both players in the room
	// so both sides can update UI
	sf::Packet resultPacket;
	resultPacket << (int)PacketType::GuessResult;
	resultPacket << room->getRoomId();
	resultPacket << player->getId();
	resultPacket << guessedNumber;
	resultPacket << resultText;
	sendPacketToRoom(resultPacket, room);

	if (correct)
	{
		room->setState(RoomState::Finished);
		room->setWinnerPlayerId(player->getId());

		player->addWin();
		opponent->addLoss();

		sf::Packet matchPacket;
		matchPacket << (int)PacketType::MatchResult;
		matchPacket << room->getRoomId();
		matchPacket << player->getId();
		matchPacket << opponent->getId();
		matchPacket << player->getName() + " guessed correctly and won!";
		sendPacketToRoom(matchPacket, room);

		sendSessionStatsUpdate(room);

		// supports the lobby event style
		broadcastLobbySystemMessage(
			player->getName() + " won against " + opponent->getName() +
			". " + player->getName() + " total win count: " + std::to_string(player->getWins()));

		// reset both players so the room can be used again for another round
		for (int pid : room->getPlayerIds())
		{
			PlayerConnection* p = getPlayerById(pid);
			if (p != nullptr)
			{
				p->setReady(false);
				p->clearSecretNumber();
			}
		}

		room->resetMatchState();
		sendRoomPlayerListUpdate(room);
		return;
	}

	// wrong guess:
	// move turn to the opponent and increment turn count
	room->incrementTurnNumber();
	room->setCurrentTurnPlayerId(opponentId);

	PlayerConnection* nextPlayer = getPlayerById(opponentId);
	std::string turnText = nextPlayer != nullptr ? nextPlayer->getName() + "'s turn!" : "Next turn";

	sf::Packet turnPacket;
	turnPacket << (int)PacketType::TurnUpdate;
	turnPacket << room->getRoomId();
	turnPacket << opponentId;
	turnPacket << room->getTurnNumber();
	turnPacket << turnText;
	sendPacketToRoom(turnPacket, room);
}

// ============================
// DISCONNECT HANDLING
// ============================
//
// when a player disconnects, the server
// - remove them from selector
// - remove them from players container
// - update their room if they were inside one
// - award opponent a win if disconnect happened during a match
// - remove an empty room
//

void handleDisconnect(int playerVectorIndex)
{
	if (playerVectorIndex < 0 || playerVectorIndex >= (int)players.size())
		return;

	PlayerConnection* player = players[playerVectorIndex].get();
	if (player == nullptr)
		return;

	std::string playerName = player->getName();
	int playerId = player->getId();
	int roomId = player->getRoomId();

	std::cout << playerName << " disconnected." << std::endl;

	// if they were inside a room, need to resolve that room's state
	if (roomId >= 0)
	{
		GameRoom* room = getRoomById(roomId);

		if (room != nullptr)
		{
			bool wasInGame = room->getState() == RoomState::InGame;
			int opponentId = getOpponentPlayerId(room, playerId);
			PlayerConnection* opponent = getPlayerById(opponentId);

			room->removePlayer(playerId);

			// if a match was active and opponent still exists,
			// opponent wins by resigning
			if (wasInGame && opponent != nullptr)
			{
				opponent->addWin();

				sf::Packet matchPacket;
				matchPacket << (int)PacketType::MatchResult;
				matchPacket << room->getRoomId();
				matchPacket << opponent->getId();
				matchPacket << playerId;
				matchPacket << opponent->getName() + " wins because opponent disconnected.";
				sendPacketToRoom(matchPacket, room);

				sendSessionStatsUpdate(room);

				broadcastLobbySystemMessage(
					opponent->getName() + " won against " + playerName +
					". " + opponent->getName() + " total win count: " + std::to_string(opponent->getWins()));

				for (int pid : room->getPlayerIds())
				{
					PlayerConnection* p = getPlayerById(pid);
					if (p != nullptr)
					{
						p->setReady(false);
						p->clearSecretNumber();
					}
				}

				room->resetMatchState();
			}

			if (!room->isEmpty())
			{
				broadcastRoomSystemMessage(room, playerName + " disconnected.");
				sendRoomPlayerListUpdate(room);
			}

			int roomIndex = findRoomIndexById(roomId);
			if (roomIndex >= 0 && rooms[roomIndex].isEmpty())
			{
				rooms.erase(rooms.begin() + roomIndex);
			}

			sendRoomListUpdateToAll();
		}
	}
	else
	{
		// player disconnected from lobby just announce it there
		broadcastLobbySystemMessage(playerName + " disconnected.");
	}

	// remove socket from selector so server stop checking
	extern sf::SocketSelector* g_selectorPtr;
	if (g_selectorPtr != nullptr)
	{
		g_selectorPtr->remove(*player->getSocket());
	}

	// remove from live players list
	players.erase(players.begin() + playerVectorIndex);
	playerCount--;
}

// pointer so disconnect helper can remove dead sockets from selector
sf::SocketSelector* g_selectorPtr = nullptr;

// ============================
// MAIN SERVER LOOP
// ============================
//
//
// 1. Start TCP listener
// 2. Wait until
//      - a new client wants to connect
//      - an existing client sent data
// 3. If listener is ready
//      - accept a new client
//      - perform handshake
//      - add them to selector and player list
// 4. For every connected player
//      - if their socket is ready receive packet
//      - read packet type
//      - send to correct handler
// 5. If a socket disconnects:
//      - call handleDisconnect()
//

int main()
{
	static sf::TcpListener listener;
	static sf::SocketSelector selector;
	g_selectorPtr = &selector;

	std::random_device rd;
	std::mt19937 gen(rd());

	if (listener.listen(53000) != sf::Socket::Status::Done)
	{
		return -1;
	}

	std::cout << "Server started on port 53000" << std::endl;

	// listener itself is added to selector so selector can tell us
	// when a new connection attempt is incoming
	selector.add(listener);

	bool isRunning = true;
	while (isRunning)
	{
		updateRoomCountdowns(gen);

		// selector.wait() blocks until one monitored socket is ready
		if (selector.wait(sf::milliseconds(100)))
		{
			// ============================
			// STEP 1: NEW CONNECTION CHECK
			// ============================
			//
			// if the listener is ready, means client is trying to connect
			if (selector.isReady(listener))
			{
				sf::TcpSocket* socket = new sf::TcpSocket();

				if (listener.accept(*socket) == sf::Socket::Status::Done)
				{
					if (handleClientConnectionRequest(socket))
					{
						players.push_back(std::make_unique<PlayerConnection>(socket, idIncrementor));
						selector.add(*socket);
						++idIncrementor;
						++playerCount;
					}
					else
					{
						socket->disconnect();
						delete socket;
					}
				}
				else
				{
					delete socket;
				}
			}

			// ============================
			// STEP 2: EXISTING PLAYER DATA
			// ============================
			//
			// loop through each connected player and check whether their socket
			// has incoming data waiting
			//
			for (int i = 0; i < (int)players.size();)
			{
				PlayerConnection* player = players[i].get();
				sf::TcpSocket* socketPtr = player->getSocket();

				if (selector.isReady(*socketPtr))
				{
					sf::Packet packet;
					sf::Socket::Status status = socketPtr->receive(packet);

					if (status == sf::Socket::Status::Done)
					{
						int type;
						packet >> type;

						// ============================
						// NAME SET AFTER HANDSHAKE
						// ============================
						//
						// if client receives its assigned ID from handshake
						// then can sen dusername
						if (type == (int)PacketType::NameSet)
						{
							std::string name;
							packet >> name;

							player->setName(name);

							std::cout << "Client " << player->getId() << " name set: " << name << std::endl;

							sf::Packet outPacket;
							outPacket << (int)PacketType::PlayerConnected;
							outPacket << player->getId();
							outPacket << player->getName();
							sendPacketToPlayer(outPacket, player);

							broadcastLobbySystemMessage(player->getName() + " joined the lobby.");
							sendRoomListUpdateToAll();
						}

						// LOBBY CHAT
						else if (type == (int)PacketType::LobbyMessageSend)
						{
							std::string msg;
							packet >> msg;

							std::string formatted = player->getName() + ": " + msg;
							std::cout << "[Lobby Chat] " << formatted << std::endl;

							sf::Packet outPacket;
							outPacket << (int)PacketType::LobbyMessageReceive;
							outPacket << player->getId();
							outPacket << formatted;
							sendPacketToLobby(outPacket);
						}

						// CREATE ROOM
						else if (type == (int)PacketType::CreateRoomRequest)
						{
							std::string roomName;
							packet >> roomName;
							handleCreateRoom(player, roomName);
						}

						// JOIN ROOM
						else if (type == (int)PacketType::JoinRoomRequest)
						{
							int roomId;
							packet >> roomId;
							handleJoinRoom(player, roomId);
						}

						// LEAVE ROOM
						else if (type == (int)PacketType::LeaveRoomRequest)
						{
							handleLeaveRoom(player);
						}

						// ROOM CHAT
						//
						// similar to lobby chat but only visible inside the room
						else if (type == (int)PacketType::RoomMessageSend)
						{
							if (player->isInLobby())
							{
								sendErrorMessage(player, "You are not inside a room.");
							}
							else
							{
								GameRoom* room = getRoomById(player->getRoomId());
								std::string msg;
								packet >> msg;

								std::string formatted = player->getName() + ": " + msg;
								std::cout << "[Room Chat] " << formatted << std::endl;

								sf::Packet outPacket;
								outPacket << (int)PacketType::RoomMessageReceive;
								outPacket << player->getId();
								outPacket << formatted;
								sendPacketToRoom(outPacket, room);
							}
						}

						// ============================
						// ROOM EMOJI
						// ============================
						//
						// emoji is handled separately so the client can decide
						// whether to render it differently from normal text chat
						else if (type == (int)PacketType::EmojiSend)
						{
							if (player->isInLobby())
							{
								sendErrorMessage(player, "Emoji can only be sent inside a room.");
							}
							else
							{
								GameRoom* room = getRoomById(player->getRoomId());
								std::string emojiText;
								packet >> emojiText;

								sf::Packet outPacket;
								outPacket << (int)PacketType::EmojiReceive;
								outPacket << player->getId();
								outPacket << emojiText;
								sendPacketToRoom(outPacket, room);
							}
						}

						// ============================
						// READY TOGGLE
						// ============================
						//
						// player can toggle ready in a room before the game starts
						// Once everyone is ready and has secret numbers chosen
						else if (type == (int)PacketType::ReadyStateChange)
						{
							if (player->isInLobby())
							{
								sendErrorMessage(player, "You are not inside a room.");
							}
							else
							{
								GameRoom* room = getRoomById(player->getRoomId());
								bool ready;
								packet >> ready;

								if (room != nullptr && room->getState() == RoomState::InGame)
								{
									sendErrorMessage(player, "Cannot change ready state during active game.");
								}
								else
								{
									player->setReady(ready);
									sendRoomPlayerListUpdate(room);
									tryStartMatchIfReady(room, gen);
								}
							}
						}

						// ============================
						// TOKEN CHANGE
						// ============================
						//
						//a player in NOTREADY state can change token
						//once READY, this is blocked
						else if (type == (int)PacketType::TokenChangeRequest)
						{
							if (player->isInLobby())
							{
								sendErrorMessage(player, "You are not inside a room.");
							}
							else
							{
								GameRoom* room = getRoomById(player->getRoomId());
								int token;
								packet >> token;

								if (player->isReady())
								{
									sendErrorMessage(player, "Cannot change token while READY.");
								}
								else
								{
									player->setToken(token);

									sf::Packet outPacket;
									outPacket << (int)PacketType::TokenChangeUpdate;
									outPacket << player->getId();
									outPacket << token;
									sendPacketToRoom(outPacket, room);

									sendRoomPlayerListUpdate(room);
								}
							}
						}

						// ===========================
						// SECRET NUMBER SUBMISSION
						// ============================
						//
						// each player chooses their own secret number
						// the opponent will later try to guess it
						//
						// happen before match start
						else if (type == (int)PacketType::SubmitSecretNumber)
						{
							if (player->isInLobby())
							{
								sendErrorMessage(player, "You are not inside a room.");
							}
							else
							{
								GameRoom* room = getRoomById(player->getRoomId());
								int secretNumber;
								packet >> secretNumber;

								if (secretNumber < SECRET_MIN || secretNumber > SECRET_MAX)
								{
									sendErrorMessage(player, "Secret number must be between 1 and 100.");
								}
								else if (player->isReady())
								{
									sendErrorMessage(player, "Cannot change secret number while READY.");
								}
								else
								{
									player->setSecretNumber(secretNumber);

									std::cout << "[Secret Debug] "
										<< player->getName()
										<< " set secret number to "
										<< secretNumber << std::endl;

									tryStartMatchIfReady(room, gen);
								}
							}
						}

						// ========================
						// GAMEPLAY GUESS
						// ===========================
						//
						// player makes a guess against opponent secret number
						// server validates then compares and advances turn/game state
						else if (type == (int)PacketType::SubmitGuess)
						{
							if (player->isInLobby())
							{
								sendErrorMessage(player, "You are not inside a room.");
							}
							else
							{
								GameRoom* room = getRoomById(player->getRoomId());
								int guessedNumber;
								packet >> guessedNumber;
								handleGuess(player, room, guessedNumber);
							}
						}
					}
					else if (status == sf::Socket::Status::Disconnected)
					{
						// if receive() says disconnected cleanup that player
						handleDisconnect(i);
						continue;
					}
				}

				++i;
			}
		}
	}

	return 0;
}