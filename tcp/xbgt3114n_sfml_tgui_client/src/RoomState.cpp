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

	// simple emoji buttons for functional testing
	// can include better UI and more emoji with art style
	emojiButtonSmile = tgui::Button::create(":smile:");
	emojiButtonSmile->setSize("10%", "6%");
	emojiButtonSmile->setPosition("2%", "90%");
	emojiButtonSmile->onPress([this]() { sendEmoji(":smile:"); });
	gui.add(emojiButtonSmile);

	emojiButtonThumbsUp = tgui::Button::create(":thumbsup:");
	emojiButtonThumbsUp->setSize("12%", "6%");
	emojiButtonThumbsUp->setPosition("13%", "90%");
	emojiButtonThumbsUp->onPress([this]() { sendEmoji(":thumbsup:"); });
	gui.add(emojiButtonThumbsUp);

	emojiButtonAngry = tgui::Button::create(":angry:");
	emojiButtonAngry->setSize("10%", "6%");
	emojiButtonAngry->setPosition("26%", "90%");
	emojiButtonAngry->onPress([this]() { sendEmoji(":angry:"); });
	gui.add(emojiButtonAngry);

	emojiButtonLaugh = tgui::Button::create(":laugh:");
	emojiButtonLaugh->setSize("10%", "6%");
	emojiButtonLaugh->setPosition("37%", "90%");
	emojiButtonLaugh->onPress([this]() { sendEmoji(":laugh:"); });
	gui.add(emojiButtonLaugh);
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
	{
		roomChatBox->addLine("[Error] Token cannot be empty");
		return;
	}

	try
	{
		int token = std::stoi(text);

		// Token isjust a logical selection id for room setup (can decorate player avatar or blabla in future)
		// allowed range now kinda small for testing so it is easier to manage
		if (token < 1 || token > 4)
		{
			roomChatBox->addLine("[Error] Token must be between 1 and 4.");
			return;
		}

		sf::Packet packet;
		packet << (int)PacketType::TokenChangeRequest;
		packet << token;
		NetworkLocator::get().send(packet);
	}
	catch (const std::exception&)
	{
		roomChatBox->addLine("[Error] Token must be a valid integer.");
	}
}

void RoomState::sendSecretNumber()
{
	auto text = secretNumberInput->getText().toStdString();
	if (text.empty())
	{
		roomChatBox->addLine("[Error] Secret number cannot be empty.");
		return;
	}

	try
	{
		int secretNumber = std::stoi(text);

		// same range rule used by the server
		if (secretNumber < 1 || secretNumber > 100)
		{
			roomChatBox->addLine("[Error] Secret number must be between 1 and 100.");
			return;
		}

		sf::Packet packet;
		packet << (int)PacketType::SubmitSecretNumber;
		packet << secretNumber;
		NetworkLocator::get().send(packet);

		// keep my own secret locally so GameplayState can display
		NetworkLocator::get().setMySecretNumber(secretNumber);

		roomChatBox->addLine("[System] My secret number is set to " + std::to_string(secretNumber) + ".");
	}
	catch (const std::exception&)
	{
		roomChatBox->addLine("[Error] Secret number must be a valid integer.");
	}
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

void RoomState::sendEmoji(const std::string& emojiText)
{
	if (emojiText.empty())
		return;

	sf::Packet packet;
	packet << (int)PacketType::EmojiSend;
	packet << emojiText;
	NetworkLocator::get().send(packet);
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

		roomChatBox->addLine("[Emoji from Player " + std::to_string(senderId) + "] " + emojiText);
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