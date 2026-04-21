#pragma once
#include "StateMachine.hpp"
#include <TGUI/TGUI.hpp>
#include <TGUI/Backend/SFML-Graphics.hpp>
#include "NetworkLocator.h"

class LobbyState : public State
{
	tgui::Gui gui; // important to be first so when need destroyed at last easy trace

	tgui::ChatBox::Ptr chatBox;
	tgui::EditBox::Ptr chatInput;
	tgui::Button::Ptr sendButton;

	tgui::ListBox::Ptr roomList;
	tgui::EditBox::Ptr roomNameInput;
	tgui::Button::Ptr createRoomButton;
	tgui::Button::Ptr joinRoomButton;

	tgui::Label::Ptr statusLabel;

	// same purpose as MenuState from below
	size_t receiveHandle = 0;

	void handlePacket(sf::Packet& packet);
	void sendLobbyMessage();
	void createRoom();
	void joinSelectedRoom();

public:
	LobbyState(StateMachine& machine, sf::RenderWindow& window, bool replace = true);
	~LobbyState();

	void activate() override;
	void pause() override;
	void resume() override;
	void handleEvent(sf::Event event) override;
	void update(sf::Time deltaTime) override;
	void draw() override;
};