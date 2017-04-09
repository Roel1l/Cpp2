#include "GameController.h"

#include <deque>
#include <vector>
#include <iterator>
#include <thread>
#include <exception>
#include <utility>
#include <chrono>

GameController::GameController()
{
}

GameController::~GameController()
{
}

void GameController::init() {
	stacks.initBuildingCards();
	stacks.initCharacterCards();

	stacks.shuffleBuildingCards();
	stacks.shuffleCharacterCards();
}

void GameController::startGame()
{
		std::this_thread::sleep_for(std::chrono::milliseconds(2000));
		sendMessageToClients("\r\n2 Players have been found starting game!\r\n", 3);
		ExecutePreparation();
		clients[0]->get_player().king = true;
		gameStage = CHOOSING_CHARACTERS;
		while (running) {
			continueGame();
		}
}

void GameController::continueGame()
{
	round++;
	switch (gameStage) {
		case PREPARATION:
			ExecutePreparation();
			break;
		case CHOOSING_CHARACTERS:
		{
			sendMessageToClients("\r\nStarting round " + std::to_string(round), 3);
			sendMessageToClients("\r\n", 3);
			if (!quickChoose) {
				int counter = 1;
				while (counter <= 4) {
					ExecuteChooseCharacters(counter);
					counter++;
				}
			}
			else {
				ExecuteChooseCharactersQuick();
			}
			gameStage = CALLING_CHARACTERS;
		}
			break;
		case CALLING_CHARACTERS:
		{
			sendMessageToClients("\r\nIt's time to start calling the characters\r\n", 3);
			ExecuteCallCharacters();
			gameStage = ENDING;
		}
			break;
		case ENDING:
			running = false;
			break;

	}
	
}

void GameController::ExecutePreparation() {
	for each (std::shared_ptr<ClientInfo> client in clients)
	{
		auto &socket = client->get_socket();
		auto &player = client->get_player();
		
		for (int i = 0; i < 4; i++)
		{
			player.buildingCards.push_back(stacks.getBuildingCard());
		}
		
		socket.write("\r\nYou have been given 2 pieces of gold and 4 building cards\r\n");
	}
}

void GameController::ExecuteChooseCharacters(int counter) {
	for each (std::shared_ptr<ClientInfo> client in clients)
	{
		auto &socket = client->get_socket();
		auto &player = client->get_player();

		if (counter == 1) {
			if (player.king) {
				currentTurnPlayerId = player.id;
				socket.write("You are the king so you get to go first\r\n");
				socket.write("The following character card will be removed: \r\n");
				socket.write(stacks.removeCharacterCard(0) + "\r\n");
				socket.write("Please choose one of the following characters to take for yourself this round:\r\n");
				socket.write(stacks.getCharacterCardOptions());
				int answer = getAnswerFromPlayer(stacks.getAmountOfCharacterCards());
				player.characterCards.push_back(stacks.getCharacterCard(answer));
				socket.write("You took the " + player.characterCards.back().name);
				socket.write("\r\n");
				switchTurns();
				return;
			}
		}
		else if(player.id == currentTurnPlayerId){
			socket.write("The other player removed a card and took one now it's your turn\r\n");
			socket.write("Choose a card to remove: \r\n");
			socket.write(stacks.getCharacterCardOptions());
			int answer = getAnswerFromPlayer(stacks.getAmountOfCharacterCards());
			socket.write(stacks.removeCharacterCard(answer) + "\r\n");
			socket.write("Please choose one of the following characters to take for yourself this round:\r\n");
			socket.write(stacks.getCharacterCardOptions());
			int secondAnswer = getAnswerFromPlayer(stacks.getAmountOfCharacterCards());
			player.characterCards.push_back(stacks.getCharacterCard(secondAnswer));
			socket.write("You took the " + player.characterCards.back().name);
			switchTurns();
			return;
		}
	}
}

void GameController::ExecuteChooseCharactersQuick() {
	sendMessageToClients("\r\nQuickChoose is turned on both players are given their characters cards\r\n", 3);

	for each (std::shared_ptr<ClientInfo> client in clients)
	{
		auto &socket = client->get_socket();
		auto &player = client->get_player();

		stacks.removeCharacterCard(0);
		player.characterCards.push_back(stacks.getCharacterCard(1));
		stacks.removeCharacterCard(0);
		player.characterCards.push_back(stacks.getCharacterCard(1));
	}
}

void GameController::ExecuteCallCharacters() {
	int callOrderId = 1;
	while (callOrderId < 9) {
		for each (std::shared_ptr<ClientInfo> client in clients) {
			auto &player = client->get_player();

			std::vector<CharacterCard>::iterator it;

			for (it = player.characterCards.begin(); it != player.characterCards.end(); it++)
			{
				if (it->id == callOrderId) {
					ExecutePlayerTurn(player, (*it));
				}
			}

		}
		callOrderId++;
	}

	sendMessageToClients("Done calling the characters. Ending round...\r\n", 3);
}

void GameController::ExecutePlayerTurn(Player & player, CharacterCard characterCard) {
	currentTurnPlayerId = player.id;
	std::string message = "It's " + player.name;
	message.append("'s turn who has the " + characterCard.name);
	sendMessageToClients(message, 3);

	bool part1Over = false;
	bool part2Over = false;
	bool characterPowerUsed = false;
	bool tookGoldOrBuilding = false;
	bool buildBuilding = false;

	while (!part1Over) {
		sendMessageToClients("\r\nWhat would you like to do?\r\n", player.id);
		message = "";
		int amountOfOptions = 0;
		int usePower = 0;
		int getGoldOrBuilding = 0;

		if (!characterPowerUsed)
		{
			amountOfOptions++;
			message.append(std::to_string(amountOfOptions) + ": Use character power.\r\n");
			usePower = amountOfOptions;
		}
		if (!tookGoldOrBuilding)
		{
			amountOfOptions++;
			message.append(std::to_string(amountOfOptions) + ": Receive 2 gold or receive a building card.\r\n");
			getGoldOrBuilding = amountOfOptions;
		}
		
		sendMessageToClients(message, player.id);
		int answer = getAnswerFromPlayer(amountOfOptions);

		if (answer == usePower) {
			PlayerUsePower(player, characterCard);
			characterPowerUsed = true;
		}
		if (answer == getGoldOrBuilding) {
			PlayerGetGoldOrBuilding(player);
			tookGoldOrBuilding = true;
			part1Over = true;
		}

	}

	while (!part2Over) {
		sendMessageToClients("\r\nWhat would you like to do?\r\n", player.id);
		message = "";
		int amountOfOptions = 0;
		int usePower = 0;
		int build = 0;
		int endTurn = 0;

		if (!characterPowerUsed)
		{
			amountOfOptions++;
			message.append(std::to_string(amountOfOptions) + ": Use character power.\r\n");
			usePower = amountOfOptions;
		}
		if (!buildBuilding)
		{
			amountOfOptions++;
			message.append(std::to_string(amountOfOptions) + ": Build a building\r\n");
			build = amountOfOptions;
		}

		amountOfOptions++;
		message.append(std::to_string(amountOfOptions) + ": End turn\r\n");
		endTurn = amountOfOptions;

		sendMessageToClients(message, player.id);
		int answer = getAnswerFromPlayer(amountOfOptions);

		if (answer == usePower) {
			PlayerUsePower(player, characterCard);
			characterPowerUsed = true;
		}
		if (answer == build) {
			PlayerBuildBuilding(player);
			buildBuilding = true;
		}
		if (answer == endTurn) {
			part2Over = true;
		}
		if (characterPowerUsed && buildBuilding) {
			part2Over = true;
		}
	}
	
	sendMessageToClients(player.name + " finished his turn as the " + characterCard.name + "\r\n", 3);
}

void GameController::PlayerGetGoldOrBuilding(Player & player) {
	std::string message = "\r\nWhich one would you like?\r\n";
	sendMessageToClients(message, player.id);
	message = "1: receive gold\r\n";
	message += "2: receive building\r\n";
	sendMessageToClients(message, player.id);

	int answer = getAnswerFromPlayer(2);

	if (answer == 1) {
		player.gold += 2;
		sendMessageToClients("\r\n" + player.name + " received 2 gold!\r\n", 3);
	}
	else if (answer == 2) {
		BuildingCard A = stacks.getBuildingCard();
		BuildingCard B = stacks.getBuildingCard();

		message = "\r\n1: " + A.name;
		message.append(" color: " + A.stringColor);
		message.append(" cost: " + std::to_string(A.cost));
		message.append("\r\n");
		
		message.append("2: " + B.name);
		message.append(" color: " + B.stringColor);
		message.append(" cost: " + std::to_string(B.cost));
		message.append("\r\n");

		sendMessageToClients(message, player.id);

		int answertwo = getAnswerFromPlayer(2);

		if (answertwo == 1) {
			player.buildingCards.push_back(A);
			stacks.discardBuildingCard(B);
		}
		else if (answertwo == 2) {
			player.buildingCards.push_back(B);
			stacks.discardBuildingCard(A);
		}

		sendMessageToClients("\r\n" + player.name + " took a building card from the stack!\r\n", 3);
	}
}

void GameController::PlayerBuildBuilding(Player & player) {

}

void GameController::PlayerUsePower(Player & player, CharacterCard characterCard) {

}

void GameController::handleClientInput(ClientCommand command)
{
	auto clientInfo = command.get_client_info().lock();
	auto &client = clientInfo->get_socket();
	auto &player = clientInfo->get_player();

	if (player.id == currentTurnPlayerId) {
		playerCommand.first = command.get_cmd();
		playerCommand.second = player.id;
	}
	else {
		if (client.is_open()) {
			client << "It's not your turn right now waiting for the other player to finish his turn...\r\n";
		}
	}
}

inline bool isInteger(const std::string & s)
{
	if (s.empty() || ((!isdigit(s[0])) && (s[0] != '-') && (s[0] != '+'))) return false;

	char * p;
	strtol(s.c_str(), &p, 10);

	return (*p == 0);
}

int GameController::getAnswerFromPlayer(int amountOfOptions) {
	bool waiting = true;
	int answer = 0;

	while (waiting) {
		if (playerCommand.first != "" && playerCommand.second == currentTurnPlayerId) {
			if (isInteger(playerCommand.first)) {

				answer = stoi(playerCommand.first);

				if (answer <= amountOfOptions && answer > 0) {
					waiting = false;
				}
				else
				{
					sendMessageToClients("Incorrect number...\r\n", currentTurnPlayerId);
				}
			}
			else {
				sendMessageToClients("Invalid input please try again...\r\n", currentTurnPlayerId);
			}

			playerCommand.first = "";
			playerCommand.second = 0;
		}
	}

	return answer;
}

void GameController::sendMessageToClients(std::string message, int playerId)
{
	for each (std::shared_ptr<ClientInfo> client in clients)
	{
		auto &socket = client->get_socket();
		auto &player = client->get_player();

		if (playerId == player.id) {
			socket.write(message);
		}
		else if (playerId > 2) {
			socket.write(message); //Als het playerId hoger dan 2 is krijgen alle clients het bericht
		}
	}
}

