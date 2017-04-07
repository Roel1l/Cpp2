#include "GameController.h"
#include "ClientCommand.h"
#include "Player.h"
#include "ClientInfo.h"

GameController::GameController()
{
}

GameController::~GameController()
{
}

void GameController::handleClientInput(ClientCommand command)
{
	auto clientInfo = command.get_client_info().lock();
	auto &client = clientInfo->get_socket();
	auto &player = clientInfo->get_player();

	continueGame(player);

	if (client.is_open()) {
		client << sendToClient;
	}
}


void GameController::continueGame(Player &player)
{
	if (player.id == currentTurnPlayerId) {

	}
	else if (player.id < 3)
		sendToClient = "Waiting for the other player it's not your turn right now";
	else
		sendToClient = "2 players are already playing right now please type 'quit' and try reconnecting another time";
}


