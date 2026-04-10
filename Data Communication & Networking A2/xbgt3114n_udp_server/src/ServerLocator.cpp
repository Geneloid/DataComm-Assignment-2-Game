#include "ServerLocator.h"
#include "Server.h"

Server* ServerLocator::m_service = nullptr;

void ServerLocator::provide(Server* service) {
	m_service = service;
}

Server& ServerLocator::get()
{
	return *m_service;
}