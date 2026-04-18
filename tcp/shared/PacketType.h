#pragma once

enum class PacketType : int
{
	HandShake = 0,
	PlayerConnected = 1,
	PlayerDisconnected = 2,
	NameSet = 3,

	// lobby-level communication
	LobbyMessageSend = 4,
	LobbyMessageReceive = 5,

	// room management
	CreateRoomRequest = 6,
	CreateRoomResponse = 7,
	JoinRoomRequest = 8,
	JoinRoomResponse = 9,
	LeaveRoomRequest = 10,
	RoomListUpdate = 11,
	RoomPlayerListUpdate = 12,

	// room-level communication
	RoomMessageSend = 13,
	RoomMessageReceive = 14,
	EmojiSend = 15,
	EmojiReceive = 16,

	// ready / token / setup
	ReadyStateChange = 17,
	ReadyStateUpdate = 18,
	TokenChangeRequest = 19,
	TokenChangeUpdate = 20,
	SubmitSecretNumber = 21,

	// game flow
	GameStartCountdown = 22,
	GameStart = 23,
	SubmitGuess = 24,
	GuessResult = 25,
	TurnUpdate = 26,
	MatchResult = 27,
	SessionStatsUpdate = 28,

	// optional error feedback
	ErrorMessage = 29
};
