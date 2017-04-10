#include "Stacks.h"
#include <fstream>
#include <string>
#include <iterator>
#include <vector>
#include <ctime>
#include <algorithm> 
#include "Globals.h"

using namespace std;

Stacks::Stacks()
{
}


Stacks::~Stacks()
{
}

void Stacks::initBuildingCards()
{
	std::ifstream input("Resources/bouwkaarten.csv");

	for (std::string line; std::getline(input, line); )
	{
		std::vector<std::string> CardInfo = split(line, ';');
		if (CardInfo.size() == 3) {
			try {
				const BuildingCard card(CardInfo[0], std::stoi(CardInfo[1]), CardInfo[2]);
				buildingCardStack.push_back(card);
			}
			catch (const std::exception& ex) {
				std::string e = ex.what();
				throw ex;
			}
		}
	}

	input.close();
}

void Stacks::initCharacterCards()
{
	std::ifstream input("Resources/karakterkaarten.csv");

	for (std::string line; std::getline(input, line); )
	{
		std::vector<std::string> CardInfo = split(line, ';');
		if (CardInfo.size() == 2) {
			try {
				const CharacterCard card(std::stoi(CardInfo[0]), CardInfo[1]);
				characterCardStack.push_back(card);
			}
			catch (const std::exception& ex) {
				std::string e = ex.what();
				throw ex;
			}
		}
	}

	input.close();
}

void Stacks::shuffleBuildingCards()
{
	srand(time(0));
	std::random_shuffle(buildingCardStack.begin(), buildingCardStack.end());
}

void Stacks::shuffleCharacterCards()
{
	srand(time(0));
	std::random_shuffle(characterCardStack.begin(), characterCardStack.end());
}

const BuildingCard Stacks::getBuildingCard() {
	if (buildingCardStack.size() == 0) {
		buildingCardStack = discardedBuildingCardStack;
		discardedBuildingCardStack.clear();
	}
	const BuildingCard returnValue = buildingCardStack.front();
	buildingCardStack.pop_front();
	return returnValue;
}

const std::string Stacks::getCharacterCardOptions()
{
	std::string options = "";
	
	deque<CharacterCard>::iterator it;

	for (it = characterCardStack.begin(); it != characterCardStack.end(); it++)
	{
		options.append(std::to_string(it - characterCardStack.begin() + 1));
		options.append(": ");
		options.append(it->name);
		options.append("\r\n");
	}

	return options;
}

const int Stacks::getAmountOfCharacterCards() {
	return characterCardStack.size();
}

void Stacks::discardBuildingCard(const BuildingCard b) {
	discardedBuildingCardStack.push_back(b);
}

const CharacterCard Stacks::getCharacterCard(const int optionId) //gebruik hier het option id van getCharacterCardOptions()
{
	CharacterCard returnCard = characterCardStack[optionId - 1];
	characterCardStack.erase(characterCardStack.begin() + optionId - 1);
	return returnCard;
}

const std::string Stacks::removeCharacterCard(const int optionId) { // optionId = 0 to remove top card
	if (optionId == 0) {
		discardedCharacterCardStack.push_back(characterCardStack.front());
		characterCardStack.pop_front();
	}
	else {
		discardedCharacterCardStack.push_back(characterCardStack[optionId - 1]);
		characterCardStack.erase(characterCardStack.begin() + optionId - 1);
	}

	return discardedCharacterCardStack.back().name;
}

void Stacks::addCharacterCard(const CharacterCard card) {
	characterCardStack.push_back(card);
}

void Stacks::undiscardCharacterCards() {

	deque<CharacterCard>::iterator it;

	for (it = discardedCharacterCardStack.begin(); it != discardedCharacterCardStack.end(); it++)
	{
		characterCardStack.push_back((*it));
	}

	discardedCharacterCardStack.clear();
}