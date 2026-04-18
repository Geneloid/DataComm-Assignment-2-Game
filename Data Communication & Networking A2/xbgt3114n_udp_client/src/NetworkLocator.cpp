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