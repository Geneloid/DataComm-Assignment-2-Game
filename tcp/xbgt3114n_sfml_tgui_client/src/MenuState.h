#pragma once
#include "StateMachine.hpp"
#include <TGUI/TGUI.hpp>
#include <TGUI/Backend/SFML-Graphics.hpp>
#include "NetworkLocator.h"

class MenuState : public State
{
	tgui::Gui gui; // important to be first

	tgui::Panel::Ptr panel;

	tgui::EditBox::Ptr nameBox;

	tgui::EditBox::Ptr ipBox1;
	tgui::EditBox::Ptr ipBox2;
	tgui::EditBox::Ptr ipBox3;
	tgui::EditBox::Ptr ipBox4;

	tgui::EditBox::Ptr portBox;

	tgui::Button::Ptr loginButton;

	// receiveHandle to keep track of my packet subscription id
	size_t receiveHandle;

	// validate input fields every time user changes text
	void onInputBoxChanged();
	void onLoginPress(); // connect to the server using the typed IP and port
	void handlePacket(sf::Packet& packet); // process packets that arrive while MenuState is active

public:

	~MenuState();

	MenuState(StateMachine& machine, sf::RenderWindow& window, bool replace = true);

	void activate() override;
	void pause() override;
	void resume() override;
	void handleEvent(sf::Event event) override;
	void update(sf::Time deltaTime) override;
	void draw() override;
};
