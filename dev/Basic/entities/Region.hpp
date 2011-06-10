/*
 * Basic Region class
 */

#pragma once

#include <vector>

#include "../constants.h"
#include "Entity.hpp"

//Signals
struct Signal {
	unsigned int id;
	Signal(unsigned int id=0) : id(id) {}
};


class Region : public Entity {
public:
	Region(unsigned int id=0);

	virtual void update();
	virtual void subscribe(sim_mob::BufferedDataManager* mgr, bool isNew) {} //Nothing for now.

private:
	std::vector<Signal> signals;
};
