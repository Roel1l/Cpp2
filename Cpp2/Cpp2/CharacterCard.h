#pragma once
#include <string>


class CharacterCard
{
public:
	CharacterCard(const int id, const std::string name);
	~CharacterCard();

	enum CharacterType {Moordenaar, Dief, Magier, Koning, Prediker, Koopman, Bouwmeester, Condottiere};

	//klasse variabelen kunnen niet const zijn omdat de shuffle functie in Stacks.cpp dit niet toelaat
	int id;
	std::string name;

	CharacterType characterType;
};

