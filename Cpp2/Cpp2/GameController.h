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
	enum GameStage {PREPARATION, CHOOSING_CHARACTERS, CALLING_CHARACTERS, ENDING};

	GameStage gameStage{ PREPARATION };

	void switchTurns() { currentTurnPlayerId = currentTurnPlayerId == 1 ? 2 : 1; };

	void ExecutePreparation();
	void ExecuteChooseCharacters(int counter);
	void ExecuteChooseCharactersQuick();
	void ExecuteCallCharacters();
	void CleanUpAfterRound();
	void ExecutePlayerTurn(Player & player, CharacterCard characterCard);
	void PlayerGetGoldOrBuilding(Player & player);
	int PlayerBuildBuilding(Player & player);

	void PlayerUsePower(Player & player, CharacterCard characterCard);
	void ExecuteMoordenaar(Player & player);
	void ExecuteDief(Player & player);
	void ExecuteMagier(Player & player);
	void ExecuteKoning(Player & player);
	void ExecutePrediker(Player & player);
	void ExecuteKoopman(Player & player);
	void ExecuteBouwmeester(Player & player);
	void ExecuteCondottiere(Player & player);

	void PlayerShowStats(Player & player);
	void ExecuteEnding();
	int CalculatePoints(Player & player);
	void sendMessageToClients(std::string message, int playerId);
	int getAnswerFromPlayer(int amountOfOptions);

	std::pair<std::string, int> playerCommand; //links is het bericht en rechts is het playerId van wie het bericht komt

	CharacterCard::CharacterType killedCharacter{ CharacterCard::CharacterType::None };
	CharacterCard::CharacterType stolenCharacter{ CharacterCard::CharacterType::None };
	int thiefPlayerId{ 0 };
	Stacks stacks;
	int currentTurnPlayerId{ 1 };
	int round{ 0 };
	bool running{ true };
	const bool quickChoose{ true };
	const int buildingsToEndGame{ 5 };
	const std::vector<std::string> characters {"Moordenaar", "Dief", "Magier", "Koning", "Prediker", "Koopman", "Bouwmeester", "Condottiere", "None"};
};

