#pragma once
#include <deque>

#include "BuildingCard.h"
#include "CharacterCard.h"


class Stacks
{
public:
	Stacks();
	~Stacks();

	void initBuildingCards();
	void initCharacterCards();
	void shuffleBuildingCards();
	void shuffleCharacterCards();

	BuildingCard getBuildingCard();

	std::string getCharacterCardOptions();

	int getAmountOfCharacterCards();

	CharacterCard getCharacterCard(int optionId);

	std::string removeCharacterCard(int optionId);

private:
	std::deque<BuildingCard> buildingCardStack;
	std::deque<CharacterCard> characterCardStack;

	std::deque<BuildingCard> discardedBuildingCardStack;
	std::deque<CharacterCard> discardedCharacterCardStack;
};

