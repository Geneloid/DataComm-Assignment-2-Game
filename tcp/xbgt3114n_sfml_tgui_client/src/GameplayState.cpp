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

	// personal tracking section
	mySecretLabel = tgui::Label::create("My Secret Number: ?");
	mySecretLabel->setPosition("52%", "2%");
	gui.add(mySecretLabel);

	myLatestGuessLabel = tgui::Label::create("My Latest Guess: none");
	myLatestGuessLabel->setPosition("52%", "7%");
	gui.add(myLatestGuessLabel);

	myGuessHistoryBox = tgui::ChatBox::create();
	myGuessHistoryBox->setSize("45%", "18%");
	myGuessHistoryBox->setPosition("52%", "12%");
	gui.add(myGuessHistoryBox);

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
	lobbyChatBox->setPosition("52%", "35%");
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
	{
		gameLogBox->addLine("[Error] Guess cannot be empty");
		return;
	}

	try
	{
		int guess = std::stoi(text);

		// match same valid range used by the server
		if (guess < 1 || guess > 100)
		{
			gameLogBox->addLine("[Error] Guess must be between 1 and 100.");
			return;
		}

		sf::Packet packet;
		packet << (int)PacketType::SubmitGuess;
		packet << guess;
		NetworkLocator::get().send(packet);

		guessInput->setText("");
	}
	catch (const std::exception&)
	{
		// if user types invalid integer text
		gameLogBox->addLine("[Error] Guess must be a valid integer.");
	}
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

		// only record private guess history if this GuessResult belongs to me
		if (guesserPlayerId == NetworkLocator::get().getLocalPlayerId())
		{
			std::string entry = std::to_string(guessedNumber) + " -> " + resultText;

			myLatestGuessLabel->setText("My Latest Guess: " + entry);
			myGuessHistoryBox->addLine(entry);
			NetworkLocator::get().addMyGuessHistory(entry);
		}
	}
	else if (type == (int)PacketType::MatchResult)
	{
		int roomId;
		int winnerPlayerId;
		int loserPlayerId;
		std::string resultMessage;

		packet >> roomId >> winnerPlayerId >> loserPlayerId >> resultMessage;

		gameLogBox->addLine("[Match Result] " + resultMessage);

		// refresh on next startup rouhnd
		NetworkLocator::get().clearMyGuessHistory();
		NetworkLocator::get().clearMySecretNumber();

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
	// if GameplayState opens will then refresh my own secret number display
	// from the locally stored session data
	int mySecret = NetworkLocator::get().getMySecretNumber();
	if (mySecret >= 0)
	{
		mySecretLabel->setText("My Secret Number: " + std::to_string(mySecret));
	}
	else
	{
		mySecretLabel->setText("My Secret Number: ?");
	}

	myGuessHistoryBox->removeAllLines();
	for (const auto& entry : NetworkLocator::get().getMyGuessHistory())
	{
		myGuessHistoryBox->addLine(entry);
	}

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