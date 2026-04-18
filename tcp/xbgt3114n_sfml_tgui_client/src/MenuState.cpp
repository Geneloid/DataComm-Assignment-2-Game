#include "MenuState.h"
#include "PacketType.h"
#include "GameState.h"

MenuState::MenuState(StateMachine& machine, sf::RenderWindow& window, bool replace)
	: State{ machine, window, replace }, gui(window)
{
	gui.loadWidgetsFromFile("../assets/form_menu.txt");

	panel = gui.get<tgui::Panel>("LoginPanel");
	nameBox = gui.get<tgui::EditBox>("UsernameBox");
	portBox = gui.get<tgui::EditBox>("PortBox");
	ipBox1 = gui.get<tgui::EditBox>("IPBox1");
	ipBox2 = gui.get<tgui::EditBox>("IPBox2");
	ipBox3 = gui.get<tgui::EditBox>("IPBox3");
	ipBox4 = gui.get<tgui::EditBox>("IPBox4");
	loginButton = gui.get<tgui::Button>("LoginButton");

	nameBox->onTextChange(&MenuState::onInputBoxChanged, this);
	portBox->onTextChange(&MenuState::onInputBoxChanged, this);
	ipBox1->onTextChange(&MenuState::onInputBoxChanged, this);
	ipBox2->onTextChange(&MenuState::onInputBoxChanged, this);
	ipBox3->onTextChange(&MenuState::onInputBoxChanged, this);
	ipBox4->onTextChange(&MenuState::onInputBoxChanged, this);
	loginButton->setEnabled(false);

	loginButton->onPress(&MenuState::onLoginPress, this);
}

MenuState::~MenuState()
{
	NetworkLocator::get().unsubscribe(receiveHandle);
}

void MenuState::onInputBoxChanged()
{
	bool ipFilled = ipBox1->getText().size() > 0 && ipBox2->getText().size() > 0 && ipBox3->getText().size() > 0 && ipBox4->getText().size() > 0;
	bool portFilled = portBox->getText().size() > 0;
	bool nameFilled = nameBox->getText().size() >= 4;
	loginButton->setEnabled(ipFilled && portFilled && nameFilled);
}

void MenuState::onLoginPress()
{
	panel->setVisible(false);

	sf::IpAddress ip(std::stoi(ipBox1->getText().toStdString()),
		std::stoi(ipBox2->getText().toStdString()),
		std::stoi(ipBox3->getText().toStdString()),
		std::stoi(ipBox4->getText().toStdString()));
	if (NetworkLocator::get().connect(ip,
		std::stoi(portBox->getText().toStdString())))
	{
		std::cout << "connected to " << ip << ":" << portBox->getText() << std::endl;
	}
}

void MenuState::handlePacket(sf::Packet& packet)
{
	int type;
	int tempIdHandshake;
	packet >> type >> tempIdHandshake;

	if (type == (int)PacketType::HandShake)
	{
		if (tempIdHandshake >= 0)
		{
			sf::Packet nameSetPacket;
			nameSetPacket << (int)PacketType::NameSet;
			nameSetPacket << nameBox->getText().toStdString();
			NetworkLocator::get().send(nameSetPacket);
		}
	}
	else if (type == (int)PacketType::PlayerConnected)
	{
		this->m_next = StateMachine::build<GameState>(m_machine, m_window, true);
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