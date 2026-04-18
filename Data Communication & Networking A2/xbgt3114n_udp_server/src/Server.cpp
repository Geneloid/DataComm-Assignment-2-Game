#include <iostream>
#include "Server.h"
#include "PacketTypes.h"
#include "PacketHeader.h"

void Server::host(unsigned short port)
{
	if (udpSocket.bind(port) == sf::Socket::Status::Done)
	{
		udpSocket.setBlocking(false);
		std::cout << "Hosted server with port: " << port << std::endl;
	}
}

void Server::send(sf::Packet& packet)
{
	for (auto& p : players)
	{
		PlayerInstance& pi = p.second;
		sf::Socket::Status status = udpSocket.send(packet, pi.getIpAddress(), pi.getPort());
	}
}

void Server::sendTo(sf::Packet& packet, std::string id)
{
	auto it = players.find(id);

	if (it != players.end())
	{
		PlayerInstance& pi = it->second;
		sendTo(packet, pi);
	}
}

void Server::sendTo(sf::Packet& packet, PlayerInstance& playerRef)
{

	PacketHeader h = { PacketType::Unreliable, 0 };
	sf::Packet wrappedPacket;
	wrappedPacket << h;
	wrappedPacket.append(packet.getData(), packet.getDataSize());

	sf::Socket::Status status = udpSocket.send(wrappedPacket, playerRef.getIpAddress(), playerRef.getPort());
}

void Server::sendReliable(sf::Packet& packet)
{
    for (auto& p : players)
        sendToReliable(packet, p.second);
}

void Server::sendToReliable(sf::Packet& packet, std::string id)
{
	auto it = players.find(id);

	if (it != players.end())
	{
		sendToReliable(packet, it->second);
	}
}

void Server::sendToReliable(sf::Packet& packet, PlayerInstance& playerRef)
{
	PacketHeader h = { PacketType::Unreliable, playerRef.getSequenceIdThenIncrement() };
	sf::Packet wrappedPacket;
	wrappedPacket << h;
	wrappedPacket.append(packet.getData(), packet.getDataSize());

	sf::Socket::Status status = 
		udpSocket.send(wrappedPacket, playerRef.getIpAddress(), playerRef.getPort());
}

void Server::update(float deltaTime)
{
	time += deltaTime;

	std::optional<sf::IpAddress> sender;
	unsigned short               senderPort = 0;
	sf::Packet packet;
	if (udpSocket.receive(packet, sender, senderPort) == sf::Socket::Status::Done)
	{
		std::string senderId = sender.value().toString() + ":" + std::to_string(senderPort);
		std::cout << "Message received from client " << senderId << std::endl;
		 
		PacketHeader receiveHeader;
		int type;
		packet >> receiveHeader >> type;

		if (receiveHeader.type == PacketType::Ack)
		{
			auto it = players.find(senderId);
			if (it != players.end())
			{
				PlayerInstance& player = it->second;
				player.receiveAckFor(receiveHeader.sequenceId);
			}
			else
			{
				std::cout << "Received ack for " << receiveHeader.sequenceId << " from " << senderId << std::endl;
			}
		}
		else if (receiveHeader.type == PacketType::Reliable)
		{
			// for some reason, client send us a reliable request
			// need to process and then response.
			// we will not be doing it in tutorial
		}
		else
		{
			if		(type == (int)RequestPacketType::ConnectRequest) {}
			else if (type == (int)RequestPacketType::DisconnectRequest) {}
			else if (type == (int)RequestPacketType::TimeSyncRequest) {}
			
		}

		if (type == (int)RequestPacketType::ConnectRequest)
		{
			// Check if requester is already connected
			auto it = std::find_if(players.begin(), players.end(), [&sender, senderPort](const std::pair<std::string, PlayerInstance>& pair)
				{
					return pair.second.getIpAddress() == sender.value() && pair.second.getPort() == senderPort;
				});

			if (it != players.end())
			{
				// player already connected, what to do?
			}
			else
			{
				std::string name;
				packet >> name;
				std::cout << "Welcoming " << name << ::std::endl;

				players.emplace(std::piecewise_construct,
					std::forward_as_tuple(senderId),
					std::forward_as_tuple(sender.value(), senderPort, senderId, name, time));

				sf::Packet outgoingPacket;
				outgoingPacket << (int)ResponsePacketType::ConnectAccepted;
				outgoingPacket << senderId;

				// this response packet could be lost in transit.
				// which means the newly created PlayerInstance (above) will be a ghost (oooOOoooOooo)
				// heartbeat check will drop this ghost player eventually
				sendTo(outgoingPacket, senderId);
				std::cout << "Sent response" << std::endl;

				playerCount++;
			}
		}
		else if (type == (int)RequestPacketType::DisconnectRequest)
		{
			// if player explicitly quit, tell everyone
			// though this isn't needed if heartbeat check is done.
		}
		else if (type == (int)RequestPacketType::TimeSyncRequest)
		{
			// if time sync, pong back
			// for this, do not update lastPacketTime

			float senderTime;
			packet >> senderTime;

			sf::Packet outgoingPacket;
			outgoingPacket << (int)ResponsePacketType::TimeSyncResponse;
			outgoingPacket << time << senderTime;
			
			sendTo(outgoingPacket, senderId);
			// TODO: change sendTo to return sf::Socket::Status so we can do
			// if(sendTo(...))
			//		std::cout << "Yay" << std::endl;
			//
			std::cout << "Sent server time" << std::endl;
		}
	}

	//	Handle RUDP pending packets for every clients

	// Iterate through all PlayerInstance, for each PI
	//		Iterate through all pi.PendingPackets, for each pending
	//			find out how long time has passed since last send
	//				if > WAIT_THRESHOLD
	//					pending.Attempts++
	//					restart timer
	//					resend packet
	//				otherwise, drop player

	static float WAIT_THRESHOLD = 0.5f;
	static int MAX_ATTEMPTS = 3;

	for (auto& [id, player] : players)
	{
		auto& pendingPackets = player.getPendingPackets();

		for (auto& [seqId, pending] : pendingPackets)
		{
			float timer = pending.timer.getElapsedTime().asSeconds();
			if (timer > WAIT_THRESHOLD)
			{
				if (pending.attempts < MAX_ATTEMPTS)
				{
					std::cout << "Resending packet with sequenceId: " << seqId << std::endl;
					pending.attempts++;
					pending.timer.restart();
					udpSocket.send(pending.packet, player.getIpAddress(), player.getPort());
				}
				else
				{
					// tried to send 3 times and not receiving ack, so drop the player
				}
			}
		}
	}

	// check heartbeat to drop player if they didn't send data for some time.
}