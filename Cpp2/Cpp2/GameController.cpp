#include "GameController.h"

#include <deque>
#include <vector>
#include <iterator>
#include <thread>
#include <exception>
#include <utility>
#include <chrono>
#include <exception>
#include <algorithm> 


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

		std::for_each(clients.begin(), clients.end(), [](std::shared_ptr<ClientInfo>& elem) {
			if(elem->get_player().id == 1) elem->get_player().king = true;
		});

		while (running) {
			if (clients.size() >= 2) {
				if (clients[0]->get_player().buildingsBuilt.size() >= buildingsToEndGame || clients[1]->get_player().buildingsBuilt.size() >= buildingsToEndGame) {
					gameStage = ENDING;
				}
			}
			continueGame();
		}
}

void GameController::continueGame()
{
	round++;
	switch (gameStage) {
		case PREPARATION:
			ExecutePreparation();
			gameStage = CHOOSING_CHARACTERS;
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
			sendMessageToClients("\r\nIt's time to start calling the characters\r\n", 3);
			ExecuteCallCharacters();
			killedCharacter = CharacterCard::CharacterType::None;
			stolenCharacter = CharacterCard::CharacterType::None;
			gameStage = CHOOSING_CHARACTERS;
			break;
		case ENDING:
			ExecuteEnding();
			break;

	}
	
}

void GameController::ExecutePreparation() {
	for each (const std::shared_ptr<ClientInfo> client in clients)
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

void GameController::ExecuteChooseCharacters(const int counter) {
	std::vector<std::shared_ptr<ClientInfo>>::iterator it;

	for (it = clients.begin(); it != clients.end(); it++)
	{
		auto &socket = it->get()->get_socket();
		auto &player = it->get()->get_player();

		if (counter == 1) {
			if (player.king) {
				currentTurnPlayerId = player.id;
				socket.write("You are the king so you get to go first\r\n");
				socket.write("The following character card will be removed: \r\n");
				socket.write(stacks.removeCharacterCard(0) + "\r\n");
				socket.write("Please choose one of the following characters to take for yourself this round:\r\n");
				socket.write(stacks.getCharacterCardOptions());
				const int answer = getAnswerFromPlayer(stacks.getAmountOfCharacterCards());
				if( answer == 0 )return;
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
			const int answer = getAnswerFromPlayer(stacks.getAmountOfCharacterCards());
			if (answer == 0)return;
			socket.write(stacks.removeCharacterCard(answer) + "\r\n");
			socket.write("Please choose one of the following characters to take for yourself this round:\r\n");
			socket.write(stacks.getCharacterCardOptions());
			const int secondAnswer = getAnswerFromPlayer(stacks.getAmountOfCharacterCards());
			if (answer == 0)return;
			player.characterCards.push_back(stacks.getCharacterCard(secondAnswer));
			socket.write("You took the " + player.characterCards.back().name + "\r\n");
			switchTurns();
			return;
		}
	}
}

void GameController::ExecuteChooseCharactersQuick() {
	sendMessageToClients("\r\nQuickChoose is turned on both players are given their characters cards\r\n", 3);

	for each (const std::shared_ptr<ClientInfo> client in clients)
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
		for each (const std::shared_ptr<ClientInfo> client in clients) {

			auto &player = client->get_player();

			std::vector<CharacterCard>::iterator it;

			for (it = player.characterCards.begin(); it != player.characterCards.end(); it++)
			{
				if (it->id == callOrderId) {
					if (it->characterType != killedCharacter) {
						ExecutePlayerTurn(player, (*it));
					}
				}
			}

		}
		callOrderId++;
	}

	sendMessageToClients("Done calling the characters. Ending round...\r\n", 3);
	CleanUpAfterRound();
}

void GameController::CleanUpAfterRound() {
	std::vector<std::shared_ptr<ClientInfo>>::iterator it;
	for (it = clients.begin(); it != clients.end(); it++)
	{
		auto &player = it->get()->get_player();

		std::vector<CharacterCard>::iterator it2;
		for (it2 = player.characterCards.begin(); it2 != player.characterCards.end(); it2++)
		{
			stacks.addCharacterCard((*it2));
		}

		player.characterCards.clear();
	}

	stacks.undiscardCharacterCards();
	stacks.shuffleCharacterCards();

}

void GameController::ExecutePlayerTurn(Player & player, const CharacterCard characterCard) {
	currentTurnPlayerId = player.id;
	std::string message = "It's " + player.name;
	message.append("'s turn who has the " + characterCard.name + "\r\n");
	sendMessageToClients(message, 3);

	if (characterCard.characterType == stolenCharacter) {
		int temp = player.gold;
		player.gold = 0;
		std::vector<std::shared_ptr<ClientInfo>>::iterator it;
		for (it = clients.begin(); it != clients.end(); it++)
		{
			auto &otherPlayer = it->get()->get_player();
			if (otherPlayer.id == thiefPlayerId) {
				otherPlayer.gold += temp;
				sendMessageToClients("\r\nThe thief stole from the "+ characterCard.name + "! " + otherPlayer.name + " got "+ std::to_string(temp) + " gold!\r\n", 3);
			}
		}
	}

	bool part1Over = false;
	bool part2Over = false;
	bool characterPowerUsed = false;
	bool tookGoldOrBuilding = false;
	bool buildBuilding = false;
	int buildingsBuilt = 0;

	while (!part1Over) {
		sendMessageToClients("\r\nWhat would you like to do?\r\n", player.id);
		message = "";
		int amountOfOptions = 0;
		int usePower = 0;
		int getGoldOrBuilding = 0;
		int showStats = 0;

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
		
		amountOfOptions++;
		message.append(std::to_string(amountOfOptions) + ": Show my info\r\n");
		showStats = amountOfOptions;

		sendMessageToClients(message, player.id);
		const int answer = getAnswerFromPlayer(amountOfOptions);
		if (answer == 0)return;

		if (answer == usePower) {
			PlayerUsePower(player, characterCard);
			characterPowerUsed = true;
		}
		if (answer == getGoldOrBuilding) {
			PlayerGetGoldOrBuilding(player);
			tookGoldOrBuilding = true;
			part1Over = true;
		}
		if (answer == showStats) {
			PlayerShowStats(player);
		}

	}

	while (!part2Over) {
		sendMessageToClients("\r\nWhat would you like to do?\r\n", player.id);
		message = "";
		int amountOfOptions = 0;
		int usePower = 0;
		int build = 0;
		int showStats = 0;
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
		message.append(std::to_string(amountOfOptions) + ": Show my info\r\n");
		showStats = amountOfOptions;

		amountOfOptions++;
		message.append(std::to_string(amountOfOptions) + ": End turn\r\n");
		endTurn = amountOfOptions;

		sendMessageToClients(message, player.id);
		const int answer = getAnswerFromPlayer(amountOfOptions);
		if (answer == 0)return;

		if (answer == usePower) {
			PlayerUsePower(player, characterCard);
			characterPowerUsed = true;
		}
		if (answer == build) {
			if (PlayerBuildBuilding(player) > 0) {
				buildingsBuilt++;
				if (characterCard.characterType == CharacterCard::CharacterType::Bouwmeester) {
					sendMessageToClients("You can build up to 3 buildings because you have the Bouwmeester!\r\n", player.id);
					if (buildingsBuilt > 2) {
						buildBuilding = true;
					}
				}
				else buildBuilding = true;
			}
		}
		if (answer == showStats) {
			PlayerShowStats(player);
		}
		if (answer == endTurn) {
			part2Over = true;
		}
		if (characterPowerUsed && buildBuilding) {
			part2Over = true;
		}
	}
	
	sendMessageToClients("\r\n" + player.name + " finished his turn as the " + characterCard.name + "\r\n", 3);
}

void GameController::PlayerGetGoldOrBuilding(Player & player) {
	std::string message = "\r\nWhich one would you like?\r\n";
	sendMessageToClients(message, player.id);
	message = "1: receive gold\r\n";
	message += "2: receive building\r\n";
	sendMessageToClients(message, player.id);

	const int answer = getAnswerFromPlayer(2);
	if (answer == 0)return;

	if (answer == 1) {
		player.gold += 2;
		sendMessageToClients("\r\n" + player.name + " received 2 gold!\r\n", 3);
	}
	else if (answer == 2) {
		try {
			const BuildingCard A = stacks.getBuildingCard();
			const BuildingCard B = stacks.getBuildingCard();

			message = "\r\n1: " + A.name;
			message.append(" color: " + A.stringColor);
			message.append(" cost: " + std::to_string(A.cost));
			message.append("\r\n");

			message.append("2: " + B.name);
			message.append(" color: " + B.stringColor);
			message.append(" cost: " + std::to_string(B.cost));
			message.append("\r\n");

			sendMessageToClients(message, player.id);

			const int answertwo = getAnswerFromPlayer(2);
			if (answertwo == 0) return;

			if (answertwo == 1) {
				player.buildingCards.push_back(A);
				stacks.discardBuildingCard(B);
			}
			else if (answertwo == 2) {
				player.buildingCards.push_back(B);
				stacks.discardBuildingCard(A);
			}

		}
		catch (const std::exception& ex) {
			std::string e = ex.what();
			sendMessageToClients("\r\nERROR: " + e + "\r\n", player.id);
		}

		sendMessageToClients("\r\n" + player.name + " took a building card from the stack!\r\n", 3);
	}
}

const int GameController::PlayerBuildBuilding(Player & player) {
	bool done = false;
	int built = 0;

	while (!done) {
		sendMessageToClients("\r\nWhich building would you like to build?\r\n", player.id);
		sendMessageToClients("You currently have: "+ std::to_string(player.gold) + " gold.\r\n", player.id);
		std::string message = "\r\n";
		std::vector<BuildingCard>::iterator it;

		for (it = player.buildingCards.begin(); it != player.buildingCards.end(); it++)
		{
			message.append(std::to_string(it - player.buildingCards.begin() + 1) + ": ");
			message.append(it->name + " ");
			message.append("color: " + it->stringColor);
			message.append(" ");
			message.append("cost: " + std::to_string(it->cost));
			message.append("\r\n");
		}

		message.append(std::to_string(player.buildingCards.size() + 1) + ": cancel.\r\n");

		sendMessageToClients(message, player.id);

		const int answer = getAnswerFromPlayer(player.buildingCards.size() + 1);
		if (answer == 0) return 0;

		if (answer == player.buildingCards.size() + 1) {
			done = true;
		}
		else {
			if (player.gold >= player.buildingCards[answer - 1].cost) {
				player.buildingsBuilt.push_back(player.buildingCards[answer - 1]);
				sendMessageToClients("\r\n" + player.name + " built the " + player.buildingCards[answer - 1].name + "!\r\n", 3);
				player.gold -= player.buildingCards[answer - 1].cost;
				player.buildingCards.erase(player.buildingCards.begin() + answer - 1);
				built++;
				done = true;
			}
			else {
				sendMessageToClients("\r\nYou don't have enough gold! you have: " + std::to_string(player.gold), player.id);
			}
		}
	}

	return built;
}

void GameController::PlayerUsePower(Player & player, const CharacterCard characterCard) {
	switch (characterCard.characterType) {
	case CharacterCard::CharacterType::Moordenaar:
		ExecuteMoordenaar(player);
		break;
	case CharacterCard::CharacterType::Dief:
		ExecuteDief(player);
		break;
	case CharacterCard::CharacterType::Magier:
		ExecuteMagier(player);
		break;
	case CharacterCard::CharacterType::Koning:
		ExecuteKoning(player);
		break;
	case CharacterCard::CharacterType::Prediker:
		ExecutePrediker(player);
		break;
	case CharacterCard::CharacterType::Koopman:
		ExecuteKoopman(player);
		break;
	case CharacterCard::CharacterType::Bouwmeester:
		ExecuteBouwmeester(player);
		break;
	case CharacterCard::CharacterType::Condottiere:
		ExecuteCondottiere(player);
		break;
	}
}

void GameController::ExecuteMoordenaar(Player & player) {
	sendMessageToClients("\r\nMoordenaar, who would you like to kill?\r\n", player.id);
	std::string message = "";

	message.append("1: Dief\r\n");
	message.append("2: Magier\r\n");
	message.append("3: Koning\r\n");
	message.append("4: Prediker\r\n");
	message.append("5: Koopman\r\n");
	message.append("6: Bouwmeester\r\n");
	message.append("7: Condotierre\r\n");

	sendMessageToClients(message, player.id);

	const int answer = getAnswerFromPlayer(7);
	if (answer == 0)return;

	switch (answer) {
	case 1:
		killedCharacter = CharacterCard::CharacterType::Dief;
		message = " Dief ";
		break;
	case 2:
		killedCharacter = CharacterCard::CharacterType::Magier;
		message = " Magier ";
		break;
	case 3:
		killedCharacter = CharacterCard::CharacterType::Koning;
		message = " Koning ";
		break;
	case 4:
		killedCharacter = CharacterCard::CharacterType::Prediker;
		message = " Prediker ";
		break;
	case 5:
		killedCharacter = CharacterCard::CharacterType::Koopman;
		message = " Koopman ";
		break;
	case 6:
		killedCharacter = CharacterCard::CharacterType::Bouwmeester;
		message = " Bouwmeester ";
		break;
	case 7:
		killedCharacter = CharacterCard::CharacterType::Condottiere;
		message = " Condottiere ";
		break;
	}

	sendMessageToClients("\r\nThe" + message + "Has been murdered and can no longer act this turn!\r\n", 3);
}

void GameController::ExecuteDief(Player & player) {
	sendMessageToClients("\r\nDief, from whom will you steal?\r\n", player.id);

	std::string message = "";
	int counter = 0;

	for (int i = CharacterCard::CharacterType::Moordenaar; i != CharacterCard::CharacterType::None; i++)
	{
		const CharacterCard::CharacterType characterType = static_cast<CharacterCard::CharacterType>(i);
		if (characterType != CharacterCard::CharacterType::Moordenaar && characterType != killedCharacter && characterType != CharacterCard::CharacterType::Dief) {
			counter++;
			message.append(std::to_string(counter) + ": " + characters[i] + "\r\n");	
		}
	}

	sendMessageToClients(message, player.id);

	const int answer = getAnswerFromPlayer(counter);
	if (answer == 0)return;

	counter = 0;

	for (int i = CharacterCard::CharacterType::Moordenaar; i != CharacterCard::CharacterType::None; i++)
	{
		const CharacterCard::CharacterType characterType = static_cast<CharacterCard::CharacterType>(i);
		if (characterType != CharacterCard::CharacterType::Moordenaar && characterType != killedCharacter && characterType != CharacterCard::CharacterType::Dief) {
			counter++;
			if (counter == answer) {
				stolenCharacter = characterType;
				thiefPlayerId = player.id;
				sendMessageToClients("\r\nThe Dief stole from the " + characters[i] + "\r\n", 3);
			}
		}
	}
}

void GameController::ExecuteMagier(Player & player) {
	sendMessageToClients("\r\nMagier, what would you like to do?\r\n", player.id);

	std::string message = "";

	message.append("1: Switch your building cards with those of the other player.\r\n");
	message.append("2: Switch a chosen amount of your building cards for new ones.\r\n");

	sendMessageToClients(message, player.id);
	const int answer = getAnswerFromPlayer(2);
	if (answer == 0)return;

	if (answer == 1) {
		std::vector<std::shared_ptr<ClientInfo>>::iterator it;
		for (it = clients.begin(); it != clients.end(); it++)
		{
			auto &otherPlayer = it->get()->get_player();
			if (otherPlayer.id != player.id) {
				std::vector<BuildingCard> temp = otherPlayer.buildingCards;

				otherPlayer.buildingCards = player.buildingCards;

				player.buildingCards = temp;

				sendMessageToClients("\r\nThe magier swapped your building cards with the other players building cards!\r\n", 3);
			}
		}
	}
	else {
		bool done = false;
		std::vector<BuildingCard> temp;

		while (!done) {
			std::vector<BuildingCard>::iterator it;
			std::string options = "\r\nWhich cards would you like to swap for new ones?\r\n";

			for (it = player.buildingCards.begin(); it != player.buildingCards.end(); it++)
			{
				options.append(std::to_string(it - player.buildingCards.begin() + 1) + ": ");
				options.append(it->name + " color: " + it->stringColor + " cost: " + std::to_string(it->cost) + "\r\n");
			}

			options.append(std::to_string(player.buildingCards.size() + 1) + ": done\r\n");

			sendMessageToClients(options, player.id);
			const int answer2 = getAnswerFromPlayer(player.buildingCards.size() + 1);
			if (answer2 == 0)return;

			if (answer2 <= player.buildingCards.size()) {
				stacks.discardBuildingCard(player.buildingCards[answer2 - 1]);
				player.buildingCards.erase(answer2 + player.buildingCards.begin() - 1);
				temp.push_back(stacks.getBuildingCard());
			}
			else 
			{
				done = true;
			}
		}

		player.buildingCards.insert(player.buildingCards.end(), temp.begin(), temp.end());
		sendMessageToClients("\r\n" + player.name + " swapped some building cards with the stack!\r\n", 3);
	}
}

void GameController::ExecuteKoning(Player & player) {

	if (!player.king) {
		player.king = true;
		sendMessageToClients("\r\n" + player.name + " has become the Koning and may go first next round!\r\n", 3);
		std::vector<std::shared_ptr<ClientInfo>>::iterator it;
		for (it = clients.begin(); it != clients.end(); it++)
		{
			auto &otherPlayer = it->get()->get_player();
			if (otherPlayer.id != player.id) {
				otherPlayer.king = false;
			}
		}
	}

	std::vector<BuildingCard>::iterator it;
	int goldEarned = 0;

	for (it = player.buildingsBuilt.begin(); it != player.buildingsBuilt.end(); it++)
	{
		if (it->color == BuildingCard::Color::GEEL) goldEarned++;
	}

	player.gold += goldEarned;
	sendMessageToClients("\r\n" + player.name + " got " + std::to_string(goldEarned) + " gold because he has " + std::to_string(goldEarned) + " yellow buildings up!\r\n", 3);
}

void GameController::ExecutePrediker(Player & player) {
	std::vector<BuildingCard>::iterator it;
	int goldEarned = 0;

	for (it = player.buildingsBuilt.begin(); it != player.buildingsBuilt.end(); it++)
	{
		if (it->color == BuildingCard::Color::BLAUW) goldEarned++;
	}

	player.gold += goldEarned;
	sendMessageToClients("\r\n" + player.name + " got " + std::to_string(goldEarned) + " gold because he has " + std::to_string(goldEarned) + " blue buildings up!\r\n", 3);
}

void GameController::ExecuteKoopman(Player & player) {
	sendMessageToClients("\r\n" + player.name + " got 1 piece of gold because he used his koopman power!\r\n", 3);
	std::vector<BuildingCard>::iterator it;
	int goldEarned = 1;

	for (it = player.buildingsBuilt.begin(); it != player.buildingsBuilt.end(); it++)
	{
		if (it->color == BuildingCard::Color::GROEN) goldEarned++;
	}

	player.gold += goldEarned;
	sendMessageToClients("\r\n" + player.name + " got " + std::to_string(goldEarned) + " gold because he has " + std::to_string(goldEarned) + " green buildings up!\r\n", 3);
}

void GameController::ExecuteBouwmeester(Player & player) {
	sendMessageToClients("\r\n" + player.name + " got 2 buildingcards because he used the bouwmeesters power!\r\n", 3);
	player.buildingCards.push_back(stacks.getBuildingCard());
	player.buildingCards.push_back(stacks.getBuildingCard());
}

void GameController::ExecuteCondottiere(Player & player) {
	std::vector<BuildingCard>::iterator it;
	int goldEarned = 0;

	for (it = player.buildingsBuilt.begin(); it != player.buildingsBuilt.end(); it++)
	{
		if (it->color == BuildingCard::Color::ROOD) goldEarned++;
	}

	player.gold += goldEarned;
	sendMessageToClients("\r\n" + player.name + " got " + std::to_string(goldEarned) + " gold because he has " + std::to_string(goldEarned) + " red buildings up!\r\n", 3);

	std::vector<std::shared_ptr<ClientInfo>>::iterator it2;
	for (it2 = clients.begin(); it2 != clients.end(); it2++)
	{
		auto &otherPlayer = it2->get()->get_player();
		if (otherPlayer.id != player.id) {
			if (otherPlayer.characterCards[0].characterType != CharacterCard::CharacterType::Prediker && otherPlayer.characterCards[1].characterType != CharacterCard::CharacterType::Prediker) {
				bool done = false;
				while (!done) {
					sendMessageToClients("\r\nCondottiere would you like to remove any of the following buildings from the other player?\r\n", player.id);
					std::string options = "";
					std::vector<BuildingCard>::iterator it3;
					for (it3 = otherPlayer.buildingsBuilt.begin(); it3 != otherPlayer.buildingsBuilt.end(); it3++)
					{
						options.append(std::to_string(it3 - otherPlayer.buildingsBuilt.begin() + 1) + ": ");
						options.append(it3->name + " color: " + it3->stringColor + " cost: " + std::to_string(it3->cost) + "\r\n");
					}

					options.append(std::to_string(otherPlayer.buildingsBuilt.size() + 1) + ": done\r\n");

					sendMessageToClients(options, player.id);
					const int answer2 = getAnswerFromPlayer(otherPlayer.buildingsBuilt.size() + 1);
					if (answer2 == 0)return;

					if (answer2 <= otherPlayer.buildingsBuilt.size()) {

						if (player.gold - otherPlayer.buildingsBuilt[answer2 - 1].cost >= 0 || otherPlayer.buildingsBuilt[answer2 - 1].cost < 2) {
							if (otherPlayer.buildingsBuilt[answer2 - 1].cost > 1) player.gold -= otherPlayer.buildingsBuilt[answer2 - 1].cost;
							sendMessageToClients("\r\nYour " + otherPlayer.buildingsBuilt[answer2 - 1].name + " was removed by the condottiere!\r\n", otherPlayer.id);
							sendMessageToClients("\r\nYou removed the other players " + otherPlayer.buildingsBuilt[answer2 - 1].name + " using the condottiere's power!\r\n", player.id);
							stacks.discardBuildingCard(otherPlayer.buildingsBuilt[answer2 - 1]);
							otherPlayer.buildingsBuilt.erase(answer2 + otherPlayer.buildingsBuilt.begin() - 1);
						}
						else
						{
							sendMessageToClients("\r\nYou don't  have enough money to remove this building.\r\n", player.id);
						}

					}
					else
					{
						done = true;
					}
				}
			}
			else {
				sendMessageToClients("\r\nThe condotierre couldn't steal from the other player because the other player has the prediker\r\n", 3);
			}
		}
	}

}

void GameController::PlayerShowStats(Player & player) {
	std::string stats = "\r\n";

	stats.append("Player Info: \r\n");
	stats.append("\r\nName: " + player.name);
	stats.append("\r\nGold: " + std::to_string(player.gold));
	stats.append("\r\nBuildings in hand: \r\n");

	std::vector<BuildingCard>::iterator it1;

	for (it1 = player.buildingCards.begin(); it1 != player.buildingCards.end(); it1++)
	{
		stats.append("    ");
		stats.append("Name: " + it1->name);
		stats.append("    ");
		stats.append("Color: " + it1->stringColor);
		stats.append("    ");
		stats.append("Cost: " + std::to_string(it1->cost));
		stats.append("\r\n");
	}

	stats.append("\r\nBuildings built: \r\n");

	std::vector<BuildingCard>::iterator it2;

	for (it2 = player.buildingsBuilt.begin(); it2 != player.buildingsBuilt.end(); it2++)
	{
		stats.append("    ");
		stats.append("Name: " + it2->name);
		stats.append("    ");
		stats.append("Color: " + it2->stringColor);
		stats.append("    ");
		stats.append("Cost: " + std::to_string(it2->cost));
		stats.append("\r\n");
	}

	stats.append("\r\nCharacters in hand: \r\n");

	std::vector<CharacterCard>::iterator it3;

	for (it3 = player.characterCards.begin(); it3 != player.characterCards.end(); it3++)
	{
		stats.append("    ");
		stats.append("Name: " + it3->name);
		stats.append("\r\n");
	}

	stats.append("\r\n");
	
	sendMessageToClients(stats, player.id);
}

void GameController::ExecuteEnding() {
	sendMessageToClients("\r\nThe Game has Ended!\r\n", 3);
	if (clients.size() >= 2) {
		auto &player1 = clients[0]->get_player();
		auto &player2 = clients[1]->get_player();
		int pointsPlayer1 = CalculatePoints(player1);
		int pointsPlayer2 = CalculatePoints(player2);


		std::string message = "";
		message.append(player1.name + " got: ");
		message.append(std::to_string(pointsPlayer1) + " points!\r\n");
		message.append("\r\n");
		message.append(player2.name + " got: ");
		message.append(std::to_string(pointsPlayer2) + " points!\r\n");
		message.append("\r\n");

		sendMessageToClients(message, 3);

		if (pointsPlayer2 > pointsPlayer1) {
			sendMessageToClients("\r\n" + player2.name + " won!!", 3);
		}
		else if (pointsPlayer2 < pointsPlayer1) {
			sendMessageToClients("\r\n" + player1.name + " won!!", 3);
		}
		else {
			sendMessageToClients("\r\nIt's a tie!", 3);
		}
	}

	sendMessageToClients("\r\nThe game has ended please type 'quit' to quit...\r\n", 3);
	running = false;
}

int GameController::CalculatePoints(Player & player) {
	int points = 0;

	bool blauw = false;
	bool rood = false;
	bool groen = false;
	bool geel = false;

	std::vector<BuildingCard>::iterator it;

	for (it = player.buildingsBuilt.begin(); it != player.buildingsBuilt.end(); it++)
	{
		points += it->cost;
		if (it->color == BuildingCard::Color::GEEL) geel = true;
		if (it->color == BuildingCard::Color::ROOD) rood = true;
		if (it->color == BuildingCard::Color::BLAUW) blauw = true;
		if (it->color == BuildingCard::Color::GROEN) groen = true;
	}
	
	if (blauw && rood && groen && geel) points += 3;
	if (player.buildingsBuilt.size() >= 8) points += 2;

	return points;
}

void GameController::handleClientInput(const ClientCommand command)
{
	auto clientInfo = command.get_client_info().lock();
	auto &client = clientInfo->get_socket();
	auto &player = clientInfo->get_player();

	if (command.get_cmd() == "stats") {
		PlayerShowStats(player);
		return;
	}
	
	if (player.id == currentTurnPlayerId) {
		playerCommand.first = command.get_cmd();
		playerCommand.second = player.id;
	}
	else {
		if (client.is_open()) {
			client << "\r\nIt's not your turn right now waiting for the other player to finish his turn...\r\n";
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

const int GameController::getAnswerFromPlayer(const int amountOfOptions) {
	bool waiting = true;
	int answer = 0;

	while (waiting && running) {
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

void GameController::sendMessageToClients(const std::string message, const int playerId)
{
	std::vector<std::shared_ptr<ClientInfo>>::iterator it;

	for (it = clients.begin(); it != clients.end(); it++)
	{
		const auto &socket = it->get()->get_socket();
		const auto &player = it->get()->get_player();

		if (playerId == player.id && running) {
			socket.write(message);
		}
		else if (playerId > 2 && running) {
			socket.write(message); //Als het playerId hoger dan 2 is krijgen alle clients het bericht
		}
	}
}

