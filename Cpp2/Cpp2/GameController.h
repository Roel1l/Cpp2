#pragma once
#include <memory>
#include <iostream>

#include "ClientCommand.h"
#include "Player.h"
#include "ClientInfo.h"
#include "Stacks.h"
#include "Player.h"
#include "ClientInfo.h"
#include "BuildingCard.h"
#include "CharacterCard.h"

class GameController
{
public:
	GameController();
	~GameController();

	void init();

	void handleClientInput(ClientCommand command);
	void startGame();

	void continueGame();

	std::vector<std::shared_ptr<ClientInfo>> clients;

private:
	enum GameStage {PREPARATION, CHOOSING_CHARACTERS, CALLING_CHARACTERS, USE_CHARACTER, ENDING};

	GameStage gameStage{ PREPARATION };

	void switchTurns() { currentTurnPlayerId = currentTurnPlayerId == 1 ? 2 : 1; };

	void ExecutePreparation();
	void checkPlayersReady();
	void sendMessageToClients(std::string message);

	Stacks stacks;
	int currentTurnPlayerId{ 1 };
	std::string sendToClient;
	std::pair<bool, int> expectingInput;
	bool bothPlayersRead{ false };
	bool running{ false };
};

