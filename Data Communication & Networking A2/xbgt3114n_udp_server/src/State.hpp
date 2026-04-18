#pragma once

namespace sf
{
	class RenderWindow;
	class Time;
	class Event;
}

class State
{
public:
	State(sf::RenderWindow& window);
	virtual ~State() = default;

	State(const State&) = delete;
	State& operator=(const State&) = delete;

	// called before state is shown.
	virtual void activate() = 0;

	// called when state is paused.
	virtual void pause() = 0;

	// called when state is resumed.
	virtual void resume() = 0;

	virtual void handleEvent(sf::Event event) = 0;
	virtual void update(sf::Time deltaTime) = 0;
	virtual void draw() = 0;

protected:
	sf::RenderWindow& m_window;
};