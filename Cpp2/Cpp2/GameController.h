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
	void ExecuteChooseCharacters(int counter);
	void ExecuteChooseCharactersQuick();
	void sendMessageToClients(std::string message, int playerId);
	int getAnswerFromPlayer(int amountOfOptions);

	std::pair<std::string, int> playerCommand; //links is het bericht en recht is het playerId

	Stacks stacks;
	int currentTurnPlayerId{ 1 };
	int round{ 0 };
	bool running{ true };
	const bool quickChoose{ true };
};

