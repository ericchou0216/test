//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

/* 
 * File:   LT_EventArgs.cpp
 * Author: Pedro Gandola <pedrogandola@smart.mit.edu>
 * 
 * Created on April 4, 2013, 5:42 PM
 */

#include "LT_EventArgs.hpp"

using namespace sim_mob::long_term;
using sim_mob::event::EventArgs;

/******************************************************************************
 *                              ExternalEventArgs   
 ******************************************************************************/
ExternalEventArgs::ExternalEventArgs(const ExternalEvent& event)
: event(event) {

}

ExternalEventArgs::ExternalEventArgs(const ExternalEventArgs& orig)
: event(orig.event) {
}

ExternalEventArgs::~ExternalEventArgs() {
}

const ExternalEvent& ExternalEventArgs::getEvent() const {
    return event;
}
