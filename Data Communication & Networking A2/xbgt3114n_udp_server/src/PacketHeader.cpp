#include "PacketHeader.h"
// SOURCE FILE

sf::Packet& operator<<(sf::Packet& packet, const PacketHeader& inHeader)
{
	return packet << (std::uint8_t)inHeader.type << inHeader.sequenceId;
}

sf::Packet& operator>>(sf::Packet& packet, PacketHeader& inoutHeader)
{
	std::uint8_t type;
	packet >> type >> inoutHeader.sequenceId;
	inoutHeader.type = (PacketType)type;

	return packet;
}