#pragma once
#include <memory>
#include <iostream>

#include "ClientCommand.h"
#include "Player.h"
#include "ClientInfo.h"

class GameController
{
public:
	GameController();
	~GameController();

	void handleClientInput(ClientCommand command);
	void continueGame(Player & player);

private:
	int currentTurnPlayerId{ 1 };
	std::string sendToClient;

	void switchTurns() { currentTurnPlayerId = currentTurnPlayerId == 1 ? 2 : 1; };
};

