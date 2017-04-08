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
			BuildingCard card(CardInfo[0], std::stoi(CardInfo[1]), CardInfo[2]);
			buildingCardStack.push_back(card);
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
			CharacterCard card(std::stoi(CardInfo[0]), CardInfo[1]);
			characterCardStack.push_back(card);
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

BuildingCard Stacks::getBuildingCard() {
	BuildingCard returnValue = buildingCardStack.front();
	buildingCardStack.pop_front();
	return returnValue;
}
