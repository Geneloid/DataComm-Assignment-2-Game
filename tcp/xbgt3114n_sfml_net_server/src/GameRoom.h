#pragma once

#include <string>
#include <vector>
#include <chrono>

enum class RoomState
{
	WaitingForPlayers = 0,
	WaitingReady = 1,
	CountdownStarting = 2,
	InGame = 3,
	Finished = 4
};

class GameRoom
{
public:
	GameRoom(int roomId, const std::string& roomName, int ownerPlayerId);

	int getRoomId() const;
	const std::string& getRoomName() const;
	int getOwnerPlayerId() const;

	const std::vector<int>& getPlayerIds() const;
	bool addPlayer(int playerId);
	bool removePlayer(int playerId);
	bool containsPlayer(int playerId) const;
	bool isFull() const;
	bool isEmpty() const;

	void setState(RoomState value);
	RoomState getState() const;

	// higher / lower per-room match state
	void resetMatchState();
	void setCurrentTurnPlayerId(int playerId);
	int getCurrentTurnPlayerId() const;

	void incrementTurnNumber();
	int getTurnNumber() const;

	void setWinnerPlayerId(int playerId);
	int getWinnerPlayerId() const;

	// countdown helpers
	void startCountdown(int seconds);
	bool isCountdownActive() const;
	bool hasCountdownFinished() const;
	void clearCountdown();

private:
	int roomId;
	std::string roomName;
	int ownerPlayerId;
	std::vector<int> playerIds;

	RoomState state = RoomState::WaitingForPlayers;

	int currentTurnPlayerId = -1;
	int turnNumber = 0;
	int winnerPlayerId = -1;

	bool countdownActive = false;
	std::chrono::steady_clock::time_point countdownEndTime;
};