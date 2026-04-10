#pragma once
// HEADER FILE

#include <SFML/Network.hpp>

// unint8 is unsigned integer where
// 8 bits are allocated
//unsigned integer is 32bits
enum class PacketType : std::uint8_t
{
	Unreliable = 0,
	Reliable = 1,
	Ack = 2
};

struct PacketHeader
{
	PacketType type;
	std::uint16_t sequenceId; 
};

// Overload << so we can push in PacketHeader into sf::Packet
sf::Packet& operator << 
	(sf::Packet& packet, const PacketHeader& inHeader);

// Overload >> so wwe can extract PacketHeader from sf::Packet
sf::Packet& operator >> 
	(sf::Packet& packet, PacketHeader& inoutHeader);