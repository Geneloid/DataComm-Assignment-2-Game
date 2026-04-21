#pragma once
#include "StateMachine.hpp"
#include <TGUI/TGUI.hpp>
#include <TGUI/Backend/SFML-Graphics.hpp>
#include "NetworkLocator.h"

class GameplayState : public State
{
	tgui::Gui gui; // important to be first so when need destroyed at last easy trace

	tgui::Label::Ptr titleLabel;
	tgui::Label::Ptr turnLabel;
	tgui::Label::Ptr resultLabel;

	tgui::Label::Ptr mySecretLabel;
	tgui::Label::Ptr myLatestGuessLabel;
	tgui::ChatBox::Ptr myGuessHistoryBox;

	tgui::EditBox::Ptr guessInput;
	tgui::Button::Ptr sendGuessButton;

	tgui::ChatBox::Ptr gameLogBox;
	tgui::ChatBox::Ptr lobbyChatBox;

	size_t receiveHandle = 0;

	// handles active match packets
		// TurnUpdate
		// GuessResult
		// MatchResult
		// RoomMessageReceive
		// LobbyMessageReceive
		// EmojiReceive
		// ErrorMessage
	void handlePacket(sf::Packet& packet);
	void sendGuess();

public:
	GameplayState(StateMachine& machine, sf::RenderWindow& window, bool replace = true);
	~GameplayState();

	void activate() override;
	void pause() override;
	void resume() override;
	void handleEvent(sf::Event event) override;
	void update(sf::Time deltaTime) override;
	void draw() override;
};