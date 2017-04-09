#pragma once
#include <string>

class BuildingCard
{
public:
	
	BuildingCard(const std::string nameIn, const int costIn, const std::string colorIn);

	~BuildingCard();

	//klasse variabelen kunnen niet const zijn omdat de shuffle functie in Stacks.cpp dit niet toelaat
	enum Color { GEEL, GROEN, BLAUW, ROOD, LILA };
	int cost;
	std::string name;
	std::string stringColor;
	Color color;

};

