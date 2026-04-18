#pragma once

enum class RequestPacketType : int
{
	Unknown = 0,
	ConnectRequest = 1,
	DisconnectRequest = 2,
	TimeSyncRequest = 3
};

enum class ResponsePacketType : int
{
	ConnectAccepted = 1,	// received when ConnectRequest is accepted
	ConnectRejected = 2,	// received when ConnectRequest is rejected
	TimeSyncResponse = 3,	// received when sending out TimeSyncRequest
	GameStateUpdate = 50,	// received periodically from server
	ServerDisconnect = 100,	// received when server shuts down gracefully

	ReliableUDPTest = 999
};