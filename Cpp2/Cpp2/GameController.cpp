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
		sendMessageToClients("\r\n2 Players have been found starting game!\r\n");
		//ExecutePreparation();
		clients[0]->get_player().king = true;
		gameStage = CHOOSING_CHARACTERS;
		while (running) {
			continueGame();
		}

}

void GameController::continueGame()
{
	switch (gameStage) {
		case PREPARATION:
			ExecutePreparation();
			break;
		case CHOOSING_CHARACTERS:
			break;
		case CALLING_CHARACTERS:
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
		
		socket.write("\r\nYou have been give 2 pieces of gold and 4 building cards\r\n");
	}
}

void GameController::handleClientInput(ClientCommand command)
{
	auto clientInfo = command.get_client_info().lock();
	auto &client = clientInfo->get_socket();
	auto &player = clientInfo->get_player();

	if (client.is_open()) {
		client << sendToClient;
	}
}

void GameController::sendMessageToClients(std::string message)
{
	for each (std::shared_ptr<ClientInfo> client in clients)
	{
		auto &socket = client->get_socket();
		socket.write(message);
	}
}
