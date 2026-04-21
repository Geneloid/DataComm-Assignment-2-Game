#include "NetworkLocator.h" // includes NetworkManager
#include "StateMachine.hpp"
#include "MenuState.h"
#include "PacketType.h"

int main()
{
	NetworkManager netManager;
	NetworkLocator::provide(&netManager);

	// window for 1024x768
	sf::RenderWindow window(sf::VideoMode({ 1024, 768 }), "Higher or Lower's Game", sf::Style::Titlebar | sf::Style::Close);

	StateMachine sm;
	sm.run(StateMachine::build<MenuState>(sm, window, true));

	sf::Clock clock;
	while (window.isOpen())
	{
		while (const std::optional event = window.pollEvent())
		{
			sm.handleEvent(*event);

			if (event->is<sf::Event::Closed>())
				window.close();
		}

		netManager.update();

		auto dt = clock.restart();

		sm.nextState();
		sm.update(dt);

		window.clear();

		sm.draw();

		window.display();
	}
}