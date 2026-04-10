#include "ServerLocator.h" // includes Server
#include <SFML/Graphics.hpp>
#include "MainState.h"

int main()
{
	Server server;
	ServerLocator::provide(&server);

	sf::RenderWindow window(sf::VideoMode({ 800, 600 }), "SFML TGUI SERVER");

	MainState mainState(window);
	mainState.activate();

	sf::Clock clock;

	server.host(54000);

	while (window.isOpen())
	{
		while (const std::optional event = window.pollEvent())
		{
			mainState.handleEvent(*event);

			if (event->is<sf::Event::Closed>())
				window.close();
		}

		auto dt = clock.restart();
		server.update(dt.asSeconds());

		mainState.update(dt);

		window.clear();

		mainState.draw();

		window.display();
	}
}