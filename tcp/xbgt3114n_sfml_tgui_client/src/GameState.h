#pragma once
#include "StateMachine.hpp"
#include <TGUI/TGUI.hpp>
#include <TGUI/Backend/SFML-Graphics.hpp>
#include "NetworkLocator.h"

class GameState : public State
{
	tgui::Gui gui; // important to be first
	tgui::Button::Ptr btn;
	tgui::ChatBox::Ptr cht;
	tgui::Label::Ptr label;
	tgui::EditBox::Ptr edt;

	size_t receiveHandle;

	void handlePacket(sf::Packet& packet);
	void sendMessage();

public:

	GameState(StateMachine& machine, sf::RenderWindow& window, bool replace = true);
	~GameState();

	void activate() override;
	void pause() override;
	void resume() override;
	void handleEvent(sf::Event event) override;
	void update(sf::Time deltaTime) override;
	void draw() override;
};
