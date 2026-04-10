#pragma once
#include "Server.h"

class ServerLocator
{
private:
	static Server* m_service;

public:
	ServerLocator() = delete;
	~ServerLocator() = delete;

	static void provide(Server* service);
	static Server& get();
};
