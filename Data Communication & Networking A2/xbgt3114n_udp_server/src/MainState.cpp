#include "MainState.h"
#include "PacketTypes.h"
#include <SFML/Window.hpp>

MainState::MainState(sf::RenderWindow& window)
	: State{ window }, gui(window)
{
	gui.loadWidgetsFromFile("../assets/form_timesync.txt");

	timeLabel = gui.get<tgui::Label>("TimeLabel");
}

MainState::~MainState() {}

void MainState::activate() {}

void MainState::pause() {}

void MainState::resume() {}

void MainState::handleEvent(sf::Event event)
{
	gui.handleEvent(event);

	if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>())
	{
		if (keyPressed->scancode == sf::Keyboard::Scancode::W)
		{
			Server& server = ServerLocator::get();

			sf::Packet p0;
			p0 << (int)ResponsePacketType::ReliableUDPTest;
			p0 << "AAAAAAAAAAAAAAAAAAAAAAA";
			server.sendReliable(p0);

			sf::Packet p1;
			p1 << (int)ResponsePacketType::ReliableUDPTest;
			p1 << "BBBBBBB";
			server.sendReliable(p1);

			sf::Packet p2;
			p2 << (int)ResponsePacketType::ReliableUDPTest;
			p2 << "CCCC";
			server.sendReliable(p2);
		}
	}
}

void MainState::update(sf::Time deltaTime)
{
	tickTime += deltaTime.asSeconds();

	Server& server = ServerLocator::get();
	float serverTime = server.getTime();
	timeLabel->setText(std::to_string(serverTime));

	circlePosition.x = 400 + 200 * cos(serverTime);
	circlePosition.y = 300 + 200 * sin(serverTime);

	if (tickTime >= tickInterval)
	{
		tickTime -= tickInterval;

		sf::Packet outPacket;

		outPacket << (int)ResponsePacketType::GameStateUpdate;
		outPacket << serverTime;
		outPacket << circlePosition.x << circlePosition.y;

		server.send(outPacket);
	}
}

void MainState::draw()
{
	m_window.clear({ 0,0,128 });
	sf::CircleShape cShape(100.0f);
	cShape.setFillColor(sf::Color::Yellow);
	cShape.setOrigin({ 100.0f, 100.0f });
	cShape.setPosition(circlePosition);
	m_window.draw(cShape);
	gui.draw();
}