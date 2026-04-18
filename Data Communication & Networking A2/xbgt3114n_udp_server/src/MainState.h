#pragma once
#include "State.hpp"
#include <TGUI/TGUI.hpp>
#include <TGUI/Backend/SFML-Graphics.hpp>
#include "ServerLocator.h"

class MainState : public State
{
	tgui::Gui gui; // important to be first

	tgui::Label::Ptr timeLabel;
	size_t receiveHandle;

	sf::Vector2f circlePosition;

	int tickRate = 10;
	float tickInterval = 1.0f / tickRate;
	float tickTime = 0.0f;

public:

	~MainState();

	MainState(sf::RenderWindow& window);

	void activate() override;
	void pause() override;
	void resume() override;
	void handleEvent(sf::Event event) override;
	void update(sf::Time deltaTime) override;
	void draw() override;
};
