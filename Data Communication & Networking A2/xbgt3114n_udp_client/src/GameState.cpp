#include "GameState.h"
#include "PacketTypes.h"

GameState::GameState(StateMachine& machine, sf::RenderWindow& window, bool replace)
	: State{ machine, window, replace }, gui(window)
{
	gui.loadWidgetsFromFile("../assets/form_timesync.txt");
	timeLabel = gui.get<tgui::Label>("TimeLabel");
}

GameState::~GameState()
{
	NetworkLocator::get().unsubscribe(receiveHandle);
}

void GameState::handlePacket(sf::Packet& packet)
{
	int type;
	packet >> type;

	// reliable shouldn't be used here, NetworkManager should handle it internally!

	if (type == (int)ResponsePacketType::GameStateUpdate)
	{
		float serverTime;
		sf::Vector2f incomingPosition;
		packet >> serverTime >> incomingPosition.x >> incomingPosition.y;

		circlePosition = incomingPosition;
		// TUTORIAL: Snapshot interpolation
	}
}

void GameState::activate()
{
	receiveHandle = NetworkLocator::get().subscribe([&](sf::Packet packet)
		{
			handlePacket(packet);
		});
}

void GameState::pause() {}

void GameState::resume() {}

void GameState::handleEvent(sf::Event event)
{
	// Pass events to TGUI
	gui.handleEvent(event);
}

void GameState::update(sf::Time deltaTime)
{
	timeLabel->setText(std::to_string(NetworkLocator::get().getTime()));

	// TUTORIAL: Snapshot interpolation
}

void GameState::draw()
{
	// Uncomment this when entering snapshot interpolation tutorial
	/*
	sf::CircleShape cShape(100.0f);
	cShape.setOrigin({ 100.0f, 100.0f });
	cShape.setPosition(circlePosition);
	m_window.draw(cShape);
	*/

	gui.draw();
}