#include "RoomState.h"
#include "PacketType.h"
#include "GameplayState.h"
#include "LobbyState.h"

#include <iostream>
#include <sstream>

RoomState::RoomState(StateMachine& machine, sf::RenderWindow& window, bool replace)
	: State{ machine, window, replace }, gui(window)
{
	// room title label
	titleLabel = tgui::Label::create("Room");
	titleLabel->setPosition("2%", "2%");
	titleLabel->setTextSize(20);
	gui.add(titleLabel);

	// countdown label
	countdownLabel = tgui::Label::create("Waiting...");
	countdownLabel->setPosition("2%", "6%");
	gui.add(countdownLabel);

	// room chat box
	roomChatBox = tgui::ChatBox::create();
	roomChatBox->setSize("45%", "38%");
	roomChatBox->setPosition("2%", "12%");
	gui.add(roomChatBox);

	// room chat input
	roomChatInput = tgui::EditBox::create();
	roomChatInput->setSize("35%", "6%");
	roomChatInput->setPosition("2%", "52%");
	gui.add(roomChatInput);

	// chat send button
	roomSendButton = tgui::Button::create("Send Room");
	roomSendButton->setSize("10%", "6%");
	roomSendButton->setPosition("38%", "52%");
	roomSendButton->onPress(&RoomState::sendRoomMessage, this);
	gui.add(roomSendButton);

	// lobby chat box
	// I keep this here because the brief wan room players can still access lobby chat
	lobbyChatBox = tgui::ChatBox::create();
	lobbyChatBox->setSize("45%", "20%");
	lobbyChatBox->setPosition("2%", "62%");
	gui.add(lobbyChatBox);

	// player list
	playerList = tgui::ListBox::create();
	playerList->setSize("45%", "30%");
	playerList->setPosition("52%", "12%");
	gui.add(playerList);

	// token input (room status)
	tokenInput = tgui::EditBox::create();
	tokenInput->setSize("18%", "6%");
	tokenInput->setPosition("52%", "46%");
	tokenInput->setDefaultText("Token");
	gui.add(tokenInput);

	setTokenButton = tgui::Button::create("Set Token");
	setTokenButton->setSize("18%", "6%");
	setTokenButton->setPosition("72%", "46%");
	setTokenButton->onPress(&RoomState::sendToken, this);
	gui.add(setTokenButton);

	secretNumberInput = tgui::EditBox::create();
	secretNumberInput->setSize("18%", "6%");
	secretNumberInput->setPosition("52%", "55%");
	secretNumberInput->setDefaultText("Secret Number");
	gui.add(secretNumberInput);

	setSecretButton = tgui::Button::create("Set Secret");
	setSecretButton->setSize("18%", "6%");
	setSecretButton->setPosition("72%", "55%");
	setSecretButton->onPress(&RoomState::sendSecretNumber, this);
	gui.add(setSecretButton);

	readyButton = tgui::Button::create("READY");
	readyButton->setSize("14%", "6%");
	readyButton->setPosition("52%", "66%");
	readyButton->onPress([this]() { sendReady(true); });
	gui.add(readyButton);

	notReadyButton = tgui::Button::create("NOT READY");
	notReadyButton->setSize("14%", "6%");
	notReadyButton->setPosition("68%", "66%");
	notReadyButton->onPress([this]() { sendReady(false); });
	gui.add(notReadyButton);

	leaveRoomButton = tgui::Button::create("Leave Room");
	leaveRoomButton->setSize("14%", "6%");
	leaveRoomButton->setPosition("84%", "66%");
	leaveRoomButton->onPress(&RoomState::leaveRoom, this);
	gui.add(leaveRoomButton);
}

RoomState::~RoomState()
{
	NetworkLocator::get().unsubscribe(receiveHandle);
}

void RoomState::sendRoomMessage()
{
	auto text = roomChatInput->getText().toStdString();
	if (text.empty())
		return;

	sf::Packet packet;
	packet << (int)PacketType::RoomMessageSend;
	packet << text;
	NetworkLocator::get().send(packet);

	roomChatInput->setText("");
}

void RoomState::sendToken()
{
	auto text = tokenInput->getText().toStdString();
	if (text.empty())
		return;

	sf::Packet packet;
	packet << (int)PacketType::TokenChangeRequest;
	packet << std::stoi(text);
	NetworkLocator::get().send(packet);
}

void RoomState::sendSecretNumber()
{
	auto text = secretNumberInput->getText().toStdString();
	if (text.empty())
		return;

	sf::Packet packet;
	packet << (int)PacketType::SubmitSecretNumber;
	packet << std::stoi(text);
	NetworkLocator::get().send(packet);
}

void RoomState::sendReady(bool readyValue)
{
	sf::Packet packet;
	packet << (int)PacketType::ReadyStateChange;
	packet << readyValue;
	NetworkLocator::get().send(packet);
}

void RoomState::leaveRoom()
{
	sf::Packet packet;
	packet << (int)PacketType::LeaveRoomRequest;
	NetworkLocator::get().send(packet);

	// directly return to LobbyState locally (room leave logic)
	this->m_next = StateMachine::build<LobbyState>(m_machine, m_window, true);
}

void RoomState::handlePacket(sf::Packet& packet)
{
	int type;
	packet >> type;

	if (type == (int)PacketType::RoomMessageReceive)
	{
		int senderId;
		std::string message;
		packet >> senderId >> message;
		roomChatBox->addLine(message);
	}
	else if (type == (int)PacketType::LobbyMessageReceive)
	{
		int senderId;
		std::string message;
		packet >> senderId >> message;
		lobbyChatBox->addLine(message);
	}
	else if (type == (int)PacketType::RoomPlayerListUpdate)
	{
		// rebuild full player list whenever server sends an update
		playerList->removeAllItems();

		int roomId;
		int playerCount;
		packet >> roomId >> playerCount;

		for (int i = 0; i < playerCount; ++i)
		{
			int playerId;
			std::string playerName;
			bool isReady;
			int token;
			int wins;
			int losses;

			packet >> playerId >> playerName >> isReady >> token >> wins >> losses;

			std::stringstream ss;
			ss << playerName
				<< " | Ready: " << (isReady ? "Yes" : "No")
				<< " | Token: " << token
				<< " | W: " << wins
				<< " | L: " << losses;

			playerList->addItem(ss.str());
		}
	}
	else if (type == (int)PacketType::TokenChangeUpdate)
	{
		int playerId;
		int token;
		packet >> playerId >> token;

		roomChatBox->addLine("[System] Player " + std::to_string(playerId) + " changed token to " + std::to_string(token));
	}
	else if (type == (int)PacketType::EmojiReceive)
	{
		int senderId;
		std::string emojiText;
		packet >> senderId >> emojiText;

		roomChatBox->addLine("[Emoji] " + emojiText);
	}
	else if (type == (int)PacketType::GameStartCountdown)
	{
		int roomId;
		int seconds;
		packet >> roomId >> seconds;

		countdownLabel->setText("Game starts in " + std::to_string(seconds) + "...");
	}
	else if (type == (int)PacketType::GameStart)
	{
		int roomId;
		packet >> roomId;

		// when server confirms match start move into GameplayState
		this->m_next = StateMachine::build<GameplayState>(m_machine, m_window, true);
	}
	else if (type == (int)PacketType::SessionStatsUpdate)
	{
		// already represented inside RoomPlayerListUpdate,
		// so for now ignored or later merge into separate UI
	}
	else if (type == (int)PacketType::ErrorMessage)
	{
		std::string errorMessage;
		packet >> errorMessage;
		roomChatBox->addLine("[Error] " + errorMessage);
	}
}

void RoomState::activate()
{
	receiveHandle = NetworkLocator::get().subscribe([&](sf::Packet packet)
		{
			handlePacket(packet);
		});
}

void RoomState::pause() {}

void RoomState::resume() {}

void RoomState::handleEvent(sf::Event event)
{
	gui.handleEvent(event);
}

void RoomState::update(sf::Time deltaTime)
{
}

void RoomState::draw()
{
	gui.draw();
}