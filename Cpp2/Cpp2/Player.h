//
//  Player.hpp
//  socketexample
//
//  Created by Bob Polis on 23-11-15.
//  Copyright © 2015 Avans Hogeschool, 's-Hertogenbosch. All rights reserved.
//

#ifndef Player_hpp
#define Player_hpp

#include <vector>
#include <string>

#include "BuildingCard.h"
#include "CharacterCard.h"

class Player {
public:
	Player(const int id, const std::string& name) : name{ name }, id{ id } {}

    std::string get_name() const { return name; }
    void set_name(const std::string& new_name) { name = new_name; }

	const int id;
	int gold{ 2 };
	std::vector<CharacterCard> characterCards;
	std::vector<BuildingCard> buildingCards;
	bool king{ false };
	std::string name;
private:
   
};

#endif /* Player_hpp */
