#include "LobbyState.h"
#include "PacketType.h"
#include "RoomState.h"

#include <iostream>
#include <sstream>

LobbyState::LobbyState(StateMachine& machine, sf::RenderWindow& window, bool replace)
	: State{ machine, window, replace }, gui(window)
{
	// status label
	statusLabel = tgui::Label::create("Lobby");
	statusLabel->setPosition("2%", "2%");
	statusLabel->setTextSize(20);
	gui.add(statusLabel);

	// lobby chat box
	chatBox = tgui::ChatBox::create();
	chatBox->setSize("58%", "55%");
	chatBox->setPosition("2%", "8%");
	gui.add(chatBox);

	// lobby chat input
	chatInput = tgui::EditBox::create();
	chatInput->setSize("46%", "6%");
	chatInput->setPosition("2%", "65%");
	gui.add(chatInput);

	// send button
	sendButton = tgui::Button::create("Send");
	sendButton->setSize("10%", "6%");
	sendButton->setPosition("50%", "65%");
	sendButton->onPress(&LobbyState::sendLobbyMessage, this);
	gui.add(sendButton);

	// room list
	roomList = tgui::ListBox::create();
	roomList->setSize("34%", "45%");
	roomList->setPosition("64%", "8%");
	gui.add(roomList);

	// room name input
	roomNameInput = tgui::EditBox::create();
	roomNameInput->setSize("22%", "6%");
	roomNameInput->setPosition("64%", "55%");
	roomNameInput->setDefaultText("Room name");
	gui.add(roomNameInput);

	// create room button
	createRoomButton = tgui::Button::create("Create Room");
	createRoomButton->setSize("16%", "6%");
	createRoomButton->setPosition("64%", "63%");
	createRoomButton->onPress(&LobbyState::createRoom, this);
	gui.add(createRoomButton);

	// join room button
	joinRoomButton = tgui::Button::create("Join Selected");
	joinRoomButton->setSize("16%", "6%");
	joinRoomButton->setPosition("82%", "63%");
	joinRoomButton->onPress(&LobbyState::joinSelectedRoom, this);
	gui.add(joinRoomButton);
}

LobbyState::~LobbyState()
{
	NetworkLocator::get().unsubscribe(receiveHandle);
}

void LobbyState::sendLobbyMessage()
{
	auto text = chatInput->getText().toStdString();
	if (text.empty())
		return;

	sf::Packet packet;
	packet << (int)PacketType::LobbyMessageSend;
	packet << text;
	NetworkLocator::get().send(packet);

	chatInput->setText("");
}

void LobbyState::createRoom()
{
	auto roomName = roomNameInput->getText().toStdString();
	if (roomName.empty())
		return;

	sf::Packet packet;
	packet << (int)PacketType::CreateRoomRequest;
	packet << roomName;
	NetworkLocator::get().send(packet);
}

void LobbyState::joinSelectedRoom()
{
	auto selected = roomList->getSelectedItem();
	if (selected.toStdString().empty())
		return;

	// expected format in list box:
	// [ID] RoomName (Players: X, State: Y)
	std::string text = selected.toStdString();

	size_t openBracket = text.find('[');
	size_t closeBracket = text.find(']');
	if (openBracket == std::string::npos || closeBracket == std::string::npos || closeBracket <= openBracket + 1)
		return;

	int roomId = std::stoi(text.substr(openBracket + 1, closeBracket - openBracket - 1));

	sf::Packet packet;
	packet << (int)PacketType::JoinRoomRequest;
	packet << roomId;
	NetworkLocator::get().send(packet);
}

void LobbyState::handlePacket(sf::Packet& packet)
{
	int type;
	packet >> type;

	if (type == (int)PacketType::LobbyMessageReceive)
	{
		int senderId;
		std::string message;
		packet >> senderId >> message;
		chatBox->addLine(message);
	}
	else if (type == (int)PacketType::RoomListUpdate)
	{
		// server sends the latest list of rooms
		// so rebuilded the room list box each time to keep it synced
		roomList->removeAllItems();

		int roomCount;
		packet >> roomCount;

		for (int i = 0; i < roomCount; ++i)
		{
			int roomId;
			std::string roomName;
			int playerCount;
			int roomState;

			packet >> roomId >> roomName >> playerCount >> roomState;

			std::stringstream ss;
			ss << "[" << roomId << "] "
				<< roomName
				<< " (Players: " << playerCount
				<< ", State: " << roomState << ")";

			roomList->addItem(ss.str());
		}
	}
	else if (type == (int)PacketType::CreateRoomResponse)
	{
		bool success;
		int roomId;
		std::string roomName;
		packet >> success >> roomId >> roomName;

		if (success)
		{
			statusLabel->setText("Created room: " + roomName);

			// if room is successfully created will move into RoomState
			this->m_next = StateMachine::build<RoomState>(m_machine, m_window, true);
		}
	}
	else if (type == (int)PacketType::JoinRoomResponse)
	{
		bool success;
		int roomId;
		packet >> success >> roomId;

		if (success)
		{
			statusLabel->setText("Joined room id: " + std::to_string(roomId));

			// sama logic once server confirms join success then transition into RoomState
			this->m_next = StateMachine::build<RoomState>(m_machine, m_window, true);
		}
	}
	else if (type == (int)PacketType::ErrorMessage)
	{
		std::string errorMessage;
		packet >> errorMessage;
		chatBox->addLine("[Error] " + errorMessage);
	}
}

void LobbyState::activate()
{
	receiveHandle = NetworkLocator::get().subscribe([&](sf::Packet packet)
		{
			handlePacket(packet);
		});
}

void LobbyState::pause() {}

void LobbyState::resume() {}

void LobbyState::handleEvent(sf::Event event)
{
	gui.handleEvent(event);
}

void LobbyState::update(sf::Time deltaTime)
{
}

void LobbyState::draw()
{
	gui.draw();
}