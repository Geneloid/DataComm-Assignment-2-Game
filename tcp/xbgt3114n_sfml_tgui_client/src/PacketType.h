#pragma once

enum class PacketType : int
{
	HandShake = 0,
	PlayerConnected = 1,
	PlayerDisconnected = 2,
	NameSet = 3,
	MessageSend = 4,
	PlayerListing = 5,
};
