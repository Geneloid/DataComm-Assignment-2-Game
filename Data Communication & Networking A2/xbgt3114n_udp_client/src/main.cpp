#include "NetworkLocator.h" // includes NetworkManager
#include "StateMachine.hpp"
#include "MenuState.h"

int main()
{
	NetworkManager netManager;
	NetworkLocator::provide(&netManager);

	sf::RenderWindow window(sf::VideoMode({ 800, 600 }), "SFML TGUI");

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

		auto dt = clock.restart();

		netManager.update(dt.asSeconds());

		sm.nextState();
		sm.update(dt);

		window.clear();

		sm.draw();

		window.display();
	}
}