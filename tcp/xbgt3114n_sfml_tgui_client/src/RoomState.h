#pragma once
#include "StateMachine.hpp"
#include <TGUI/TGUI.hpp>
#include <TGUI/Backend/SFML-Graphics.hpp>
#include "NetworkLocator.h"

class RoomState : public State
{
	tgui::Gui gui; // important to be first so when need destroyed at last easy trace

	tgui::Label::Ptr titleLabel;
	tgui::Label::Ptr countdownLabel;

	tgui::ChatBox::Ptr roomChatBox;
	tgui::EditBox::Ptr roomChatInput;
	tgui::Button::Ptr roomSendButton;

	tgui::ChatBox::Ptr lobbyChatBox;

	tgui::ListBox::Ptr playerList;

	tgui::EditBox::Ptr tokenInput;
	tgui::Button::Ptr setTokenButton;

	tgui::EditBox::Ptr secretNumberInput;
	tgui::Button::Ptr setSecretButton;

	tgui::Button::Ptr readyButton;
	tgui::Button::Ptr notReadyButton;
	tgui::Button::Ptr leaveRoomButton;

	size_t receiveHandle = 0;

	// handle all room related packets
		//  RoomMessageReceive
		//  LobbyMessageReceive
		//  TokenChangeUpdate
		//  GameStartCountdown
		//  GameStart
		//  ErrorMessage
	void handlePacket(sf::Packet& packet);

	void sendRoomMessage();
	void sendToken();
	void sendSecretNumber();
	void sendReady(bool readyValue);
	void leaveRoom();

public:
	RoomState(StateMachine& machine, sf::RenderWindow& window, bool replace = true);
	~RoomState();

	void activate() override;
	void pause() override;
	void resume() override;
	void handleEvent(sf::Event event) override;
	void update(sf::Time deltaTime) override;
	void draw() override;
};