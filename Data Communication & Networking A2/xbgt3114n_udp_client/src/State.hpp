#pragma once

#include <memory>

class StateMachine;

namespace sf
{
	class RenderWindow;
	class Time;
	class Event;
}

class State
{
public:
	State(StateMachine& machine, sf::RenderWindow& window, bool replace = true);
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

	std::unique_ptr<State> next();

	// is this state replacing any currently running State, or overlaying instead?
	[[nodiscard]] bool isReplacing() const;

protected:
	StateMachine& m_machine;
	sf::RenderWindow& m_window;

	bool m_replacing;

	std::unique_ptr<State> m_next;
};