#include "MenuState.h"
#include "PacketType.h"
#include "LobbyState.h"

// use this flag for quick same - PC testing
// true, the menu will auto - fill localhost connection values
static const bool AUTO_CONNECT_LOCAL = true;

MenuState::MenuState(StateMachine& machine, sf::RenderWindow& window, bool replace)
	: State{ machine, window, replace }, gui(window)
{
	// menu layout from TGUI builder here
	gui.loadWidgetsFromFile("../assets/form_menu.txt");

	panel = gui.get<tgui::Panel>("LoginPanel");
	nameBox = gui.get<tgui::EditBox>("UsernameBox");
	portBox = gui.get<tgui::EditBox>("PortBox");
	ipBox1 = gui.get<tgui::EditBox>("IPBox1");
	ipBox2 = gui.get<tgui::EditBox>("IPBox2");
	ipBox3 = gui.get<tgui::EditBox>("IPBox3");
	ipBox4 = gui.get<tgui::EditBox>("IPBox4");
	loginButton = gui.get<tgui::Button>("LoginButton");

	// only becomes clickable when all important fields are filled
	nameBox->onTextChange(&MenuState::onInputBoxChanged, this);
	portBox->onTextChange(&MenuState::onInputBoxChanged, this);
	ipBox1->onTextChange(&MenuState::onInputBoxChanged, this);
	ipBox2->onTextChange(&MenuState::onInputBoxChanged, this);
	ipBox3->onTextChange(&MenuState::onInputBoxChanged, this);
	ipBox4->onTextChange(&MenuState::onInputBoxChanged, this);
	loginButton->setEnabled(false);

	loginButton->onPress(&MenuState::onLoginPress, this);

	// same-PC local testing will auto-fill localhost and default server port
	if (AUTO_CONNECT_LOCAL)
	{
		ipBox1->setText("127");
		ipBox2->setText("0");
		ipBox3->setText("0");
		ipBox4->setText("1");
		portBox->setText("53000");

		// temporary placeholder for default username for quick testing
		nameBox->setText("Connecting...");

		// re-check inputs after auto-filling so login button becomes enabled
		onInputBoxChanged();

		// auto-connect immediately
		// but need server already running on the same PC
		onLoginPress();
	}
}

MenuState::~MenuState()
{
	// once this state is destroyed will unsubscribe from NetworkManager
	NetworkLocator::get().unsubscribe(receiveHandle);
}

void MenuState::onInputBoxChanged()
{
	// split the IP into 4 boxes, so only allow login when ada 4 sections
	bool ipFilled = ipBox1->getText().size() > 0 && ipBox2->getText().size() > 0 && ipBox3->getText().size() > 0 && ipBox4->getText().size() > 0;
	bool portFilled = portBox->getText().size() > 0;

	// minimum username length incase tiny empty-like names
	bool nameFilled = nameBox->getText().size() >= 4;
	loginButton->setEnabled(ipFilled && portFilled && nameFilled);
}

void MenuState::onLoginPress()
{
	// once login is pressed hide panel first
	// if connection fail can re-show it later from error handling
	panel->setVisible(false);

	try
	{
		int ip1 = std::stoi(ipBox1->getText().toStdString());
		int ip2 = std::stoi(ipBox2->getText().toStdString());
		int ip3 = std::stoi(ipBox3->getText().toStdString());
		int ip4 = std::stoi(ipBox4->getText().toStdString());
		int port = std::stoi(portBox->getText().toStdString());

		// basic validation so clearly invalid numbers are blocked early
		if (ip1 < 0 || ip1 > 255 ||
			ip2 < 0 || ip2 > 255 ||
			ip3 < 0 || ip3 > 255 ||
			ip4 < 0 || ip4 > 255)
		{
			std::cout << "[Menu Error] IP address sections must be between 0 and 255." << std::endl;
			panel->setVisible(true);
			return;
		}

		if (port <= 0 || port > 65535)
		{
			std::cout << "[Menu Error] Port must be between 1 and 65535." << std::endl;
			panel->setVisible(true);
			return;
		}

		sf::IpAddress ip(ip1, ip2, ip3, ip4);

		if (NetworkLocator::get().connect(ip, static_cast<unsigned short>(port)))
		{
			std::cout << "Connected to " << ip << ":" << port << std::endl;
		}
		else
		{
			std::cout << "[Menu Error] Failed to connect to server." << std::endl;
			panel->setVisible(true);
		}
	}
	catch (const std::exception&)
	{
		std::cout << "[Menu Error] IP and port must be valid integers." << std::endl;
		panel->setVisible(true);
	}

	// connect() only means TCP connection succeed
	// the server might not fully accepted the player yet
	// depends on HandShake from the server
}

void MenuState::handlePacket(sf::Packet& packet)
{
	int type;
	packet >> type;

	if (type == (int)PacketType::HandShake)
	{
		int tempIdHandshake;
		packet >> tempIdHandshake;

		if (tempIdHandshake >= 0)
		{
			// server-assigned id to generate a clear testing name
			// like
			// id 0 -> TestPlayer1
			// id 1 -> TestPlayer2
			// id 2 -> TestPlayer3
			// and so on
			//
			// for we can see who joined in logs and chat
			std::string generatedName = "TestPlayer" + std::to_string(tempIdHandshake + 1);

			// update menu field too so I can visually see which name this client got
			nameBox->setText(generatedName);

			sf::Packet nameSetPacket;
			nameSetPacket << (int)PacketType::NameSet;
			nameSetPacket << generatedName;
			NetworkLocator::get().send(nameSetPacket);
		}
	}
	else if (type == (int)PacketType::PlayerConnected)
	{
		// if server accepts NameSet, it sends PlayerConnected back
		int playerId;
		std::string playerName;
		packet >> playerId >> playerName;

		std::cout << "Server accepted player " << playerName << " with id " << playerId << std::endl;

		// store this local client is so later states can identify
		// my own packets, my own guesses, and my own UI section
		NetworkLocator::get().setLocalPlayerInfo(playerId, playerName);

		this->m_next = StateMachine::build<LobbyState>(m_machine, m_window, true);
	}
	else if (type == (int)PacketType::ErrorMessage)
	{
		std::string errorMessage;
		packet >> errorMessage;
		std::cout << "[Menu Error] " << errorMessage << std::endl;

		// if handshake/login failed for some reason show panel again
		panel->setVisible(true);
	}
}

void MenuState::activate()
{
	receiveHandle = NetworkLocator::get().subscribe([&](sf::Packet packet)
		{
			handlePacket(packet);
		});
}

void MenuState::pause() {}

void MenuState::resume() {}

void MenuState::handleEvent(sf::Event event)
{
	gui.handleEvent(event);
}

void MenuState::update(sf::Time deltaTime)
{
}

void MenuState::draw()
{
	gui.draw();
}