#include "GameRoom.h"
#include <algorithm>
#include <chrono>

GameRoom::GameRoom(int roomId, const std::string& roomName, int ownerPlayerId)
{
	this->roomId = roomId;
	this->roomName = roomName;
	this->ownerPlayerId = ownerPlayerId;
	this->playerIds.push_back(ownerPlayerId);
}

int GameRoom::getRoomId() const
{
	return roomId;
}

const std::string& GameRoom::getRoomName() const
{
	return roomName;
}

int GameRoom::getOwnerPlayerId() const
{
	return ownerPlayerId;
}

const std::vector<int>& GameRoom::getPlayerIds() const
{
	return playerIds;
}

bool GameRoom::addPlayer(int playerId)
{
	if (containsPlayer(playerId) || isFull())
		return false;

	playerIds.push_back(playerId);
	return true;
}

bool GameRoom::removePlayer(int playerId)
{
	auto it = std::find(playerIds.begin(), playerIds.end(), playerId);
	if (it == playerIds.end())
		return false;

	playerIds.erase(it);
	return true;
}

bool GameRoom::containsPlayer(int playerId) const
{
	return std::find(playerIds.begin(), playerIds.end(), playerId) != playerIds.end();
}

bool GameRoom::isFull() const
{
	// 2-player minimum is required, and game idea is built around 2 players
	// so lock it to 2 players for easier implementation
	return playerIds.size() >= 2;
}

bool GameRoom::isEmpty() const
{
	return playerIds.empty();
}

void GameRoom::setState(RoomState value)
{
	state = value;
}

RoomState GameRoom::getState() const
{
	return state;
}

void GameRoom::resetMatchState()
{
	currentTurnPlayerId = -1;
	turnNumber = 0;
	winnerPlayerId = -1;
	state = RoomState::WaitingReady;
	countdownActive = false;
}

void GameRoom::setCurrentTurnPlayerId(int playerId)
{
	currentTurnPlayerId = playerId;
}

int GameRoom::getCurrentTurnPlayerId() const
{
	return currentTurnPlayerId;
}

void GameRoom::incrementTurnNumber()
{
	++turnNumber;
}

int GameRoom::getTurnNumber() const
{
	return turnNumber;
}

void GameRoom::setWinnerPlayerId(int playerId)
{
	winnerPlayerId = playerId;
}

int GameRoom::getWinnerPlayerId() const
{
	return winnerPlayerId;
}

void GameRoom::startCountdown(int seconds)
{
	countdownActive = true;
	countdownEndTime = std::chrono::steady_clock::now() + std::chrono::seconds(seconds);
}

bool GameRoom::isCountdownActive() const
{
	return countdownActive;
}

bool GameRoom::hasCountdownFinished() const
{
	if (!countdownActive)
		return false;

	return std::chrono::steady_clock::now() >= countdownEndTime;
}

void GameRoom::clearCountdown()
{
	countdownActive = false;
}