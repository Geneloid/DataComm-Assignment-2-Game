#include "NetworkLocator.h"
#include "NetworkManager.h"

NetworkManager* NetworkLocator::m_service = nullptr;

void NetworkLocator::provide(NetworkManager* service) {
	m_service = service;
}

NetworkManager& NetworkLocator::get()
{
	return *m_service;
}

void NetworkManager::setLocalPlayerInfo(int id, const std::string& name)
{
	localPlayerId = id;
	localPlayerName = name;
}

int NetworkManager::getLocalPlayerId() const
{
	return localPlayerId;
}

const std::string& NetworkManager::getLocalPlayerName() const
{
	return localPlayerName;
}

void NetworkManager::setMySecretNumber(int value)
{
	mySecretNumber = value;
}

int NetworkManager::getMySecretNumber() const
{
	return mySecretNumber;
}

void NetworkManager::clearMySecretNumber()
{
	mySecretNumber = -1;
}

void NetworkManager::addMyGuessHistory(const std::string& entry)
{
	myGuessHistory.push_back(entry);
}

const std::vector<std::string>& NetworkManager::getMyGuessHistory() const
{
	return myGuessHistory;
}

void NetworkManager::clearMyGuessHistory()
{
	myGuessHistory.clear();
}