#include "GameplayState.h"
#include "PacketType.h"
#include "RoomState.h"

#include <iostream>

GameplayState::GameplayState(StateMachine& machine, sf::RenderWindow& window, bool replace)
	: State{ machine, window, replace }, gui(window)
{
	titleLabel = tgui::Label::create("Gameplay");
	titleLabel->setPosition("2%", "2%");
	titleLabel->setTextSize(20);
	gui.add(titleLabel);

	turnLabel = tgui::Label::create("Waiting for turn...");
	turnLabel->setPosition("2%", "7%");
	gui.add(turnLabel);

	resultLabel = tgui::Label::create("No guess yet.");
	resultLabel->setPosition("2%", "12%");
	gui.add(resultLabel);

	guessInput = tgui::EditBox::create();
	guessInput->setSize("20%", "6%");
	guessInput->setPosition("2%", "20%");
	guessInput->setDefaultText("Enter guess 1-100");
	gui.add(guessInput);

	sendGuessButton = tgui::Button::create("Submit Guess");
	sendGuessButton->setSize("14%", "6%");
	sendGuessButton->setPosition("24%", "20%");
	sendGuessButton->onPress(&GameplayState::sendGuess, this);
	gui.add(sendGuessButton);

	gameLogBox = tgui::ChatBox::create();
	gameLogBox->setSize("45%", "45%");
	gameLogBox->setPosition("2%", "30%");
	gui.add(gameLogBox);

	lobbyChatBox = tgui::ChatBox::create();
	lobbyChatBox->setSize("45%", "20%");
	lobbyChatBox->setPosition("52%", "30%");
	gui.add(lobbyChatBox);
}

GameplayState::~GameplayState()
{
	NetworkLocator::get().unsubscribe(receiveHandle);
}

void GameplayState::sendGuess()
{
	auto text = guessInput->getText().toStdString();
	if (text.empty())
		return;

	sf::Packet packet;
	packet << (int)PacketType::SubmitGuess;
	packet << std::stoi(text);
	NetworkLocator::get().send(packet);

	guessInput->setText("");
}

void GameplayState::handlePacket(sf::Packet& packet)
{
	int type;
	packet >> type;

	if (type == (int)PacketType::TurnUpdate)
	{
		int roomId;
		int currentTurnPlayerId;
		int turnNumber;
		std::string uiText;

		packet >> roomId >> currentTurnPlayerId >> turnNumber >> uiText;
		turnLabel->setText(uiText + " (Turn " + std::to_string(turnNumber) + ")");
		gameLogBox->addLine("[Turn] " + uiText);
	}
	else if (type == (int)PacketType::GuessResult)
	{
		int roomId;
		int guesserPlayerId;
		int guessedNumber;
		std::string resultText;

		packet >> roomId >> guesserPlayerId >> guessedNumber >> resultText;

		resultLabel->setText("Guess " + std::to_string(guessedNumber) + ": " + resultText);
		gameLogBox->addLine("Player " + std::to_string(guesserPlayerId) + " guessed " + std::to_string(guessedNumber) + " -> " + resultText);
	}
	else if (type == (int)PacketType::MatchResult)
	{
		int roomId;
		int winnerPlayerId;
		int loserPlayerId;
		std::string resultMessage;

		packet >> roomId >> winnerPlayerId >> loserPlayerId >> resultMessage;

		gameLogBox->addLine("[Match Result] " + resultMessage);

		// after match ends return to room state for next round
		this->m_next = StateMachine::build<RoomState>(m_machine, m_window, true);
	}
	else if (type == (int)PacketType::LobbyMessageReceive)
	{
		int senderId;
		std::string message;
		packet >> senderId >> message;
		lobbyChatBox->addLine(message);
	}
	else if (type == (int)PacketType::RoomMessageReceive)
	{
		int senderId;
		std::string message;
		packet >> senderId >> message;
		gameLogBox->addLine(message);
	}
	else if (type == (int)PacketType::EmojiReceive)
	{
		int senderId;
		std::string emojiText;
		packet >> senderId >> emojiText;
		gameLogBox->addLine("[Emoji] " + emojiText);
	}
	else if (type == (int)PacketType::ErrorMessage)
	{
		std::string errorMessage;
		packet >> errorMessage;
		gameLogBox->addLine("[Error] " + errorMessage);
	}
}

void GameplayState::activate()
{
	receiveHandle = NetworkLocator::get().subscribe([&](sf::Packet packet)
		{
			handlePacket(packet);
		});
}

void GameplayState::pause() {}

void GameplayState::resume() {}

void GameplayState::handleEvent(sf::Event event)
{
	gui.handleEvent(event);
}

void GameplayState::update(sf::Time deltaTime)
{
}

void GameplayState::draw()
{
	gui.draw();
}