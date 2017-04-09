#include "GameController.h"

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
		continueGame();
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
			int i = 3;
		}
			break;
		case USE_CHARACTER:
			break;
		case ENDING:
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
			client << "It's not your turn right now waiting for the other player to finish his turn...";
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

