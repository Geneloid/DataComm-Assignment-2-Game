#include "GameState.h"
#include "PacketType.h"

GameState::GameState(StateMachine& machine, sf::RenderWindow& window, bool replace)
	: State{ machine, window, replace }, gui(window)
{
	gui.loadWidgetsFromFile("../assets/form_game.txt");

	gui.get<tgui::Button>("Button1")->onPress(&GameState::sendMessage, this);
	cht = gui.get<tgui::ChatBox>("ChatBox1");
	edt = gui.get<tgui::EditBox>("EditBox1");
}

GameState::~GameState()
{
	NetworkLocator::get().unsubscribe(receiveHandle);
}

void GameState::handlePacket(sf::Packet& packet)
{
	int type;
	packet >> type;
	if (type == (int)PacketType::MessageSend)
	{
		std::string message;
		packet >> message;
		cht->addLine(message);
	}
}

void GameState::sendMessage()
{
	auto& text = edt->getText();

	if (text.size() > 0)
	{
		sf::Packet packet;

		packet << (int)PacketType::MessageSend;
		packet << text.toStdString();

		NetworkLocator::get().send(packet);
		edt->setText("");
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
}

void GameState::draw()
{
	gui.draw();
}