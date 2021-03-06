//Copyright (c) 2016 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//license.txt   (http://opensource.org/licenses/MIT)


/*
 * AwakeningSubModel.cpp
 *
 *  Created on: 1 Feb 2016
 *  Author: Chetan Rogbeer <chetan.rogbeer@smart.mit.edu>
 */

#include <model/AwakeningSubModel.hpp>
#include "core/LoggerAgent.hpp"
#include "core/AgentsLookup.hpp"
#include "role/impl/HouseholdBidderRole.hpp"
#include "role/impl/HouseholdSellerRole.hpp"
#include <stdlib.h>
#include <vector>
#include "util/PrintLog.hpp"
#include "util/SharedFunctions.hpp"
#include "agent/impl/HouseholdAgent.hpp"

using namespace std;

namespace sim_mob
{
    namespace long_term
    {
        AwakeningSubModel::AwakeningSubModel() {}

        AwakeningSubModel::~AwakeningSubModel() {}

        double AwakeningSubModel::getFutureTransitionOwn()
        {
            return futureTransitionOwn;
        }

        bool  AwakeningSubModel::ComputeFutureTransition(Household *household, HM_Model *model, double &futureTransitionRate, double &futureTransitionRandomDraw )
        {
            std::string tenureTransitionId="";
            //The age category were set by the Jingsi shaw (xujs@mit.edu)
            //in her tenure transition model.
            if( household->getAgeOfHead() <= 6 )
                tenureTransitionId = "<35";
            else
            if( household->getAgeOfHead()>= 7 && household->getAgeOfHead() <= 9 )
                tenureTransitionId = "35-49";
            else
            if( household->getAgeOfHead()>= 10 && household->getAgeOfHead() <= 12 )
                tenureTransitionId = "50-64";
            else
            if( household->getAgeOfHead()>= 13 )
                tenureTransitionId = "65+";

            string tenureStatus;

            if( household->getTenureStatus() == 1 ) //owner
                tenureStatus = "own";
            else
                tenureStatus = "rent";

            for(int p = 0; p < model->getTenureTransitionRatesSize(); p++)
            {
                if( model->getTenureTransitionRates(p)->getCurrentStatus() == tenureStatus &&
                    model->getTenureTransitionRates(p)->getFutureStatus() == string("own") &&
                    model->getTenureTransitionRates(p)->getAgeGroup() == tenureTransitionId )
                {
                    futureTransitionRate = model->getTenureTransitionRates(p)->getRate() / 100.0;
                }
            }

            futureTransitionRandomDraw = (double)rand()/RAND_MAX;

            if( futureTransitionRandomDraw < futureTransitionRate )
                futureTransitionOwn = true; //Future transition is to OWN a unit

            //own->own: proceed as normal
            //own->rent: randomly choose a rental unit for this unit.
            //rent->own: proceed as normal
            //rent->rent: do nothing
            if( futureTransitionOwn == false )//futureTransition is to RENT
            {
                if( household->getTenureStatus() == 2) //renter
                {
                    //rent->rent: do nothing
                    //agent goes inactive
                }
                else
                {
                    //own->rent
                    //This awakened household will now look for a rental unit
                    //Let's change its tenure status here.
                    household->setTenureStatus(2);
                }

                return false;
            }
            else
                return true;
        }

        void AwakeningSubModel::InitialAwakenings(HM_Model *model, Household *household, HouseholdAgent *agent, int day)
        {
            ConfigParams& config = ConfigManager::GetInstanceRW().FullConfig();
            if (config.ltParams.resume)
            {
                return;
            }
            boost::mutex::scoped_lock lock( mtx );

            if( agent->getId() >= model->FAKE_IDS_START )
                return;

            HouseholdBidderRole *bidder = agent->getBidder();
            HouseholdSellerRole *seller = agent->getSeller();

            if( bidder == nullptr || seller == nullptr )
            {
                printError((boost::format( "The bidder or seller classes is null.")).str());
                return;
            }



            if(household == nullptr)
            {
                printError((boost::format( "household  of agent %1% is null.") %  agent->getId()).str());
                return;
            }


            //We will awaken a specific number of households on day 1 as dictated by the long term XML file.
            if( model->getAwakeningCounter() > config.ltParams.housingModel.awakeningModel.initialHouseholdsOnMarket)
                return;

            if( config.ltParams.housingModel.awakeningModel.awakenModelRandom == true )
            {
                float random = (float)rand() / RAND_MAX;

                if( random < 0.5 )
                    return;


                printAwakening(day, household);

                #ifdef VERBOSE
                PrintOutV("[day " << day << "] Lifestyle 3. Household " << household->getId() << " has been awakened. " << model->getNumberOfBidders() << std::endl);
                #endif

                model->incrementLifestyle3HHs();
            }
            else
            if( config.ltParams.housingModel.awakeningModel.awakenModelShan == true )
            {
                Awakening *awakening = model->getAwakeningById( household->getId() );

                const float equalClassProb = 0.33;
                const float baseAwakeningProb = 0.02;


                printError((boost::format( "The awakening object is null for household %1%. We'll set it to the average.") % household->getId()).str());

                float class1 = equalClassProb;
                float class2 = equalClassProb;
                float class3 = equalClassProb;
                float awaken_class1 = baseAwakeningProb;
                float awaken_class2 = baseAwakeningProb;
                float awaken_class3 = baseAwakeningProb;

                if( awakening == nullptr )
                {
                    class1 = awakening->getClass1();
                    class2 = awakening->getClass2();
                    class3 = awakening->getClass3();
                    awaken_class1 = awakening->getAwakenClass1();
                    awaken_class2 = awakening->getAwakenClass2();
                    awaken_class3 = awakening->getAwakenClass3();
                }


                float r1 = (float)rand() / RAND_MAX;
                int lifestyle = 1;

                if( r1 > class1 && r1 <= class1 + class2 )
                {
                    lifestyle = 2;
                }
                else if( r1 > class1 + class2 )
                {
                    lifestyle = 3;
                }

                float r2 = (float)rand() / RAND_MAX;

                int ageCategory = 0;

                r2 = r2 * movingProbability(household, model, true);

                IdVector unitIds = agent->getUnitIds();

                if( lifestyle == 1 && r2 < awaken_class1)
                {

                    printAwakening(day, household);

                    #ifdef VERBOSE
                    PrintOutV("[day " << day << "] Lifestyle 1. Household " << household->getId() << " has been awakened." << model->getNumberOfBidders()  << std::endl);
                    #endif

                    model->incrementLifestyle1HHs();
                }
                else
                if( lifestyle == 2 && r2 < awaken_class2)
                {

                    printAwakening(day, household);

                    #ifdef VERBOSE
                    PrintOutV("[day " << day << "] Lifestyle 2. Household " << household->getId() << " has been awakened. "  << model->getNumberOfBidders() << std::endl);
                    #endif

                    model->incrementLifestyle2HHs();
                }
                else
                if( lifestyle == 3 && r2 < awaken_class3)
                {

                    printAwakening(day, household);

                    #ifdef VERBOSE
                    PrintOutV("[day " << day << "] Lifestyle 3. Household " << household->getId() << " has been awakened. " << model->getNumberOfBidders() << std::endl);
                    #endif

                    model->incrementLifestyle3HHs();
                }

            }
            else
            if( config.ltParams.housingModel.awakeningModel.awakenModelJingsi== true )
            {
                double movingRate = movingProbability(household, model, true ) / 100.0;

                double randomDrawMovingRate = (double)rand()/RAND_MAX;

                if( randomDrawMovingRate > movingRate )
                    return;

                double futureTransitionRate = 0.0;
                double futureTransitionRandomDraw = 0.0;

                bool success = ComputeFutureTransition(household, model, futureTransitionRate, futureTransitionRandomDraw );

                if( success == false )
                    return;

                printAwakeningJingsi(day, household, futureTransitionRate, futureTransitionRandomDraw, movingRate, randomDrawMovingRate);

                #ifdef VERBOSE
                PrintOutV("[day " << day << "] Lifestyle 3. Household " << household->getId() << " has been awakened. " << model->getNumberOfBidders() << std::endl);
                #endif
            }

            IdVector unitIds = agent->getUnitIds();

            for (vector<BigSerial>::const_iterator itr = unitIds.begin(); itr != unitIds.end(); itr++)
            {
                BigSerial unitId = *itr;
                Unit* unit = const_cast<Unit*>(model->getUnitById(unitId));

                unit->setbiddingMarketEntryDay(day);
                unit->setTimeOnMarket( 1 + config.ltParams.housingModel.timeOnMarket);
                unit->setTimeOffMarket( 1 + config.ltParams.housingModel.timeOffMarket);
            }

            bidder->setActive(true);
            household->setAwakenedDay(0);
            household->setLastBidStatus(0);
            household->setLastAwakenedDay(0);
            int householdBiddingWindow = ( config.ltParams.housingModel.householdBiddingWindow ) * (double)rand() / RAND_MAX + 1;
            household->setTimeOnMarket(householdBiddingWindow);
            agent->setHouseholdBiddingWindow(householdBiddingWindow);
            //note :: what happens if a household never bids during the bidding window?? where do we set the time off market for those households?
            model->incrementAwakeningCounter();
        }


        double AwakeningSubModel::movingProbability( Household* household, HM_Model *model, bool day0)
        {
            std::vector<BigSerial> individuals = household->getIndividuals();
            Individual *householdHead;
            for(int n = 0; n < individuals.size(); n++ )
            {
                Individual *individual = model->getIndividualById( individuals[n] );

                if(individual->getHouseholdHead())
                    householdHead = individual;
            }

            for(int n = 0; n < model->getOwnerTenantMovingRatesSize(); n++)
            {
                bool awake_day0 = model->getOwnerTenantMovingRates(n)->getDayZero();
                int ageCategory = model->getOwnerTenantMovingRates(n)->getAgeCategory();

                if( householdHead->getAgeCategoryId() == ageCategory )
                {
                    if( awake_day0 == day0 )
                    {
                        if( household->getTenureStatus() == 2 )
                            return model->getOwnerTenantMovingRates(n)->getTenantMovingPercentage();
                        else
                            return model->getOwnerTenantMovingRates(n)->getOwnerMovingPercentage();
                    }
                }
            }

            return 1.0;
        }


        std::vector<ExternalEvent> AwakeningSubModel::DailyAwakenings( int day, HM_Model *model)
        {
            std::vector<ExternalEvent> events;
            ConfigParams& config = ConfigManager::GetInstanceRW().FullConfig();

            int dailyAwakenings = config.ltParams.housingModel.awakeningModel.dailyHouseholdAwakenings;

            int n = 0;

            for( ; n < dailyAwakenings; )
            {
                ExternalEvent extEv;

                BigSerial householdId = (double)rand()/RAND_MAX * model->getHouseholdList()->size();

                Household *household = model->getHouseholdById(householdId);

                if (!household)
                    continue;

                int awakenDay = household->getLastAwakenedDay();

                if (household->getLastBidStatus() == 0 && day < awakenDay + household->getTimeOnMarket())
                    continue;

                if (household->getLastBidStatus() == 1)
                {       
                    if (awakenDay + config.ltParams.housingModel.awakeningModel.awakeningOffMarketSuccessfulBid < day)
                        continue;
                }

                if (household->getLastBidStatus() == 2)
                {
                    if (awakenDay +  config.ltParams.housingModel.awakeningModel.awakeningOffMarketUnsuccessfulBid < day)
                        continue;
                }

                if (household->getPendingStatusId() == 1)
                    continue;

                double futureTransitionRate = 0;
                double futureTransitionRandomDraw = 0;

                bool success = ComputeFutureTransition(household, model, futureTransitionRate, futureTransitionRandomDraw );

                if (success == false)
                    continue;

                double movingRate = movingProbability(household, model, false ) / 100.0;

                double movingRateRandomDraw = (double)rand()/RAND_MAX;

                if (movingRateRandomDraw > movingRate)
                    continue;

                AgentsLookup& lookup = AgentsLookupSingleton::getInstance();
                const HouseholdAgent *householdAgent = lookup.getHouseholdAgentById(household->getId());    

                if(householdAgent == nullptr)
                    continue;

                n++;

                household->setLastBidStatus(0);
                household->setAwakenedDay(day);
                household->setLastAwakenedDay(day);
                int householdBiddingWindow = config.ltParams.housingModel.householdBiddingWindow;
                household->setTimeOnMarket(householdBiddingWindow);

                (const_cast<HouseholdAgent*>(householdAgent))->setHouseholdBiddingWindow(householdBiddingWindow);
                
                model->incrementAwakeningCounter();
                printAwakeningJingsi(day, household, futureTransitionRate, futureTransitionRandomDraw, movingRate, movingRateRandomDraw);

                extEv.setDay(day + 1);
                extEv.setType(ExternalEvent::NEW_JOB);
                extEv.setHouseholdId(household->getId());
                extEv.setDeveloperId(0);

                events.push_back(extEv);
            }

            int simYear = config.ltParams.year;
            std::tm currentDate = getDateBySimDay(simYear,day);
            HM_Model::HouseholdList pendingHouseholds = model->getPendingHouseholds();
            for(Household *household : pendingHouseholds)
            {
                AgentsLookup& lookup = AgentsLookupSingleton::getInstance();
                const HouseholdAgent *householdAgent = lookup.getHouseholdAgentById(household->getId());
                const Unit *newUnit = householdAgent->getModel()->getUnitById(household->getUnitPending());

                if(newUnit != nullptr)
                {
                    boost::gregorian::date moveInDate = boost::gregorian::date_from_tm(newUnit->getOccupancyFromDate());
                    boost::gregorian::date simulationDate(HITS_SURVEY_YEAR, 1, 1);
                    boost::gregorian::date_duration dt(day);
                    simulationDate = simulationDate + dt;
                    int moveInWaitingTimeInDays = (moveInDate - simulationDate).days();

                    if(moveInWaitingTimeInDays <= config.ltParams.housingModel.housingMoveInDaysInterval)
                    {
                        //set the last bid status to 1 as this house has already done a successful bid and waiting to move in.
                        household->setLastBidStatus(1);
                        household->setAwakenedDay(day);
                        household->setLastAwakenedDay(day);

                        IdVector unitIds = householdAgent->getUnitIds();

                        for (vector<BigSerial>::const_iterator itr = unitIds.begin(); itr != unitIds.end(); itr++)
                        {
                            BigSerial unitId = *itr;
                            Unit* unit = const_cast<Unit*>(model->getUnitById(unitId));

                            unit->setbiddingMarketEntryDay(day);
                            unit->setTimeOnMarket( 1 + config.ltParams.housingModel.timeOnMarket);
                            unit->setTimeOffMarket( 1 + config.ltParams.housingModel.timeOffMarket);
                        }
                    }
                }
            }

            return events;
        }
    }
}
