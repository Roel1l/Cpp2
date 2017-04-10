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

	const BuildingCard getBuildingCard();

	const std::string getCharacterCardOptions();

	const int getAmountOfCharacterCards();

	void discardBuildingCard(const BuildingCard b);

	const CharacterCard getCharacterCard(const int optionId);

	const std::string removeCharacterCard(const int optionId);

	void addCharacterCard(const CharacterCard card);

	void undiscardCharacterCards();

private:
	std::deque<BuildingCard> buildingCardStack;
	std::deque<CharacterCard> characterCardStack;

	std::deque<BuildingCard> discardedBuildingCardStack;
	std::deque<CharacterCard> discardedCharacterCardStack;
};

