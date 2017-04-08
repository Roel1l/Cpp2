#include "BuildingCard.h"



BuildingCard::BuildingCard(const std::string name, const int cost, std::string colorIn) : name{ name }, cost{ cost }
{
	if (colorIn == "groen") {
		color = GROEN;
	}
	else if (colorIn == "blauw") {
		color = BLAUW;
	}
	else if (colorIn == "geel") {
		color = GEEL;
	}
	else if (colorIn == "rood") {
		color = ROOD;
	}
	else if (colorIn == "lila") {
		color = LILA;
	}
}


BuildingCard::~BuildingCard()
{
}
