//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

/**
 * \file main.cpp
 * A first approximation of the basic pseudo-code in C++. The main file loads several
 * properties from data/config.xml, and attempts a simulation run. Currently, the various
 * granularities and pedestrian starting locations are loaded.
 *
 * \author Seth N. Hetu
 * \author LIM Fung Chai
 * \author Xu Yan
 */
#include <vector>
#include <string>
#include <ctime>

//main.cpp (top-level) files can generally get away with including GenConfig.h
#include "GenConfig.h"

#include "workers/Worker.hpp"
#include "buffering/BufferedDataManager.hpp"
#include "workers/WorkGroup.hpp"
#include "geospatial/aimsun/Loader.hpp"
#include "geospatial/RoadNetwork.hpp"
#include "geospatial/MultiNode.hpp"
#include "geospatial/UniNode.hpp"
#include "geospatial/LaneConnector.hpp"
#include "geospatial/RoadSegment.hpp"
#include "geospatial/Lane.hpp"
#include "util/OutputUtil.hpp"
#include "util/DailyTime.hpp"
#include "entities/signal/Signal.hpp"


#include "conf/simpleconf.hpp"
#include "entities/AuraManager.hpp"
#include "entities/TrafficWatch.hpp"
#include "entities/Person.hpp"
#include "entities/roles/Role.hpp"
#include "entities/roles/RoleFactory.hpp"
#include "entities/roles/activityRole/ActivityPerformer.hpp"
#include "entities/roles/driver/BusDriver.hpp"
#include "entities/roles/driver/Driver.hpp"
#include "entities/roles/pedestrian/Pedestrian.hpp"
#include "entities/roles/pedestrian/Pedestrian2.hpp"
#include "entities/roles/passenger/Passenger.hpp"
#include "entities/profile/ProfileBuilder.hpp"
#include "geospatial/BusStop.hpp"
#include "geospatial/Roundabout.hpp"
#include "geospatial/Intersection.hpp"
#include "geospatial/Route.hpp"
#include "perception/FixedDelayed.hpp"
#include "buffering/Buffered.hpp"
#include "buffering/Locked.hpp"
#include "buffering/Shared.hpp"


//add by xuyan
#include "partitions/PartitionManager.hpp"
#include "partitions/ParitionDebugOutput.hpp"

//Note: This must be the LAST include, so that other header files don't have
//      access to cout if SIMMOB_DISABLE_OUTPUT is true.
#include <iostream>
#include <tinyxml.h>

using std::cout;
using std::endl;
using std::vector;
using std::string;

using namespace sim_mob;

//Start time of program
timeval start_time;

//Helper for computing differences. May be off by ~1ms
namespace {
int diff_ms(timeval t1, timeval t2) {
    return (((t1.tv_sec - t2.tv_sec) * 1000000) + (t1.tv_usec - t2.tv_usec))/1000;
}
} //End anon namespace

//Current software version.
const string SIMMOB_VERSION = string(SIMMOB_VERSION_MAJOR) + ":" + SIMMOB_VERSION_MINOR;



/**
 * Main simulation loop.
 * \note
 * For doxygen, we are setting the variable JAVADOC AUTOBRIEF to "true"
 * This isn't necessary for class-level documentation, but if we want
 * documentation for a short method (like "get" or "set") then it makes sense to
 * have a few lines containing brief/full comments. (See the manual's description
 * of JAVADOC AUTOBRIEF). Of course, we can discuss this first.
 *
 * \par
 * See Buffered.hpp for an example of this in action.
 *
 * \par
 * ~Seth
 *
 * This function is separate from main() to allow for easy scoping of WorkGroup objects.
 */
bool performMain(const std::string& configFileName,const std::string& XML_OutPutFileName) {
	cout <<"Starting SimMobility, version1 " <<SIMMOB_VERSION <<endl;
	
	ProfileBuilder* prof = nullptr;
#ifdef SIMMOB_AGENT_UPDATE_PROFILE
	ProfileBuilder::InitLogFile("agent_update_trace.txt");
	ProfileBuilder prof_i;
	prof = &prof_i;
#endif

	//Register our Role types.
	//TODO: Accessing ConfigParams before loading it is technically safe, but we
	//      should really be clear about when this is not ok.
	for (int i=0; i<2; i++) {
		//Set for the old-style config first, new-style config second.
		RoleFactory& rf = (i==0) ? ConfigParams::GetInstance().getRoleFactoryRW() : Config::GetInstanceRW().roleFactory();
		MutexStrategy mtx = (i==0) ? ConfigParams::GetInstance().mutexStategy : Config::GetInstance().mutexStrategy();

		rf.registerRole("driver", new sim_mob::Driver(nullptr, mtx));
		rf.registerRole("pedestrian", new sim_mob::Pedestrian2(nullptr));
		//rf.registerRole("passenger",new sim_mob::Passenger(nullptr));

		rf.registerRole("passenger",new sim_mob::Passenger(nullptr, mtx));
		rf.registerRole("busdriver", new sim_mob::BusDriver(nullptr, mtx));
		rf.registerRole("activityRole", new sim_mob::ActivityPerformer(nullptr));
		//rf.registerRole("buscontroller", new sim_mob::BusController()); //Not a role!
	}

	//Loader params for our Agents
	WorkGroup::EntityLoadParams entLoader(Agent::pending_agents, Agent::all_agents);

	//Prepare our built-in models
	//NOTE: These can leak memory for now, but don't delete them because:
	//      A) If a built-in Construct is used then it will need these models.
	//      B) We'll likely replace these with Factory classes later (static, etc.), so
	//         memory management will cease to be an issue.
	Config::BuiltInModels builtIn;
	builtIn.carFollowModels["mitsim"] = new MITSIM_CF_Model();
	builtIn.laneChangeModels["mitsim"] = new MITSIM_LC_Model();
	builtIn.intDrivingModels["linear"] = new SimpleIntDrivingModel();

	//Load our user config file
	ConfigParams::InitUserConf(configFileName, Agent::all_agents, Agent::pending_agents, prof, builtIn);

	//Save a handle to the shared definition of the configuration.
	const ConfigParams& config = ConfigParams::GetInstance();

	//Start boundaries
	if (!config.MPI_Disabled() && config.is_run_on_many_computers) {
		PartitionManager::instance().initBoundaryTrafficItems();
	}

	bool NoDynamicDispatch = config.DynamicDispatchDisabled();

	PartitionManager* partMgr = nullptr;
	if (!config.MPI_Disabled() && config.is_run_on_many_computers) {
		partMgr = &PartitionManager::instance();
	}

	{ //Begin scope: WorkGroups
	//TODO: WorkGroup scope currently does nothing. We need to re-enable WorkGroup deletion at some later point. ~Seth

	//Work Group specifications
	WorkGroup* agentWorkers = WorkGroup::NewWorkGroup(config.agentWorkGroupSize, config.totalRuntimeTicks, config.granAgentsTicks, &AuraManager::instance(), partMgr);
	WorkGroup* signalStatusWorkers = WorkGroup::NewWorkGroup(config.signalWorkGroupSize, config.totalRuntimeTicks, config.granSignalsTicks);

	//Initialize all work groups (this creates barriers, and locks down creation of new groups).
	WorkGroup::InitAllGroups();

	//Initialize each work group individually
	agentWorkers->initWorkers(NoDynamicDispatch ? nullptr :  &entLoader);
	signalStatusWorkers->initWorkers(nullptr);

	//Anything in all_agents is starting on time 0, and should be added now.
	for (vector<Entity*>::iterator it = Agent::all_agents.begin(); it != Agent::all_agents.end(); it++) {
		agentWorkers->assignAWorker(*it);
	}

	//Assign all signals too
	for (vector<Signal*>::iterator it = Signal::all_signals_.begin(); it != Signal::all_signals_.end(); it++) {
//		std::cout << "performmain() Signal " << (*it)->getId() << "  Has " <<  (*it)->getPhases().size()/* << "  " << (*it)->getNOF_Phases()*/ <<  " phases\n";
		signalStatusWorkers->assignAWorker(*it);
	}

	cout << "Initial Agents dispatched or pushed to pending." << endl;

	//Initialize the aura manager
	AuraManager::instance().init();

	///
	///  TODO: Do not delete this next line. Please read the comment in TrafficWatch.hpp
	///        ~Seth
	///
//	TrafficWatch& trafficWatch = TrafficWatch::instance();

	//Start work groups and all threads.
	WorkGroup::StartAllWorkGroups();

	//
	if (!config.MPI_Disabled() && config.is_run_on_many_computers) {
		PartitionManager& partitionImpl = PartitionManager::instance();
		partitionImpl.setEntityWorkGroup(agentWorkers, signalStatusWorkers);

		std::cout << "partition_solution_id in main function:" << partitionImpl.partition_config->partition_solution_id << std::endl;
	}

	/////////////////////////////////////////////////////////////////
	// NOTE: WorkGroups are able to handle skipping steps by themselves.
	//       So, we simply call "wait()" on every tick, and on non-divisible
	//       time ticks, the WorkGroups will return without performing
	//       a barrier sync.
	/////////////////////////////////////////////////////////////////
	size_t numStartAgents = Agent::all_agents.size();
	size_t numPendingAgents = Agent::pending_agents.size();
	size_t maxAgents = Agent::all_agents.size();

	timeval loop_start_time;
	gettimeofday(&loop_start_time, nullptr);
	int loop_start_offset = diff_ms(loop_start_time, start_time);

	ParitionDebugOutput debug;

	///START DEBUG
	const StreetDirectory& stdir = StreetDirectory::instance();

	//TODO: Find the two Bus Stops closest to node 58950 (two different sides of Victoria Street)
	//      and plan a path from one to the other (will ensure they walk around the long way).

	//TODO: It seems that *all* paths are reversed.
	const BusStop* from = stdir.getBusStop(Point2D(37290071,14390219));
	//const Node* from = ConfigParams::GetInstance().getNetwork().locateNode(Point2D(37241080,14362955)); //61688
	const Node* to = ConfigParams::GetInstance().getNetwork().locateNode(Point2D(37270984,14378959)); //45666

	vector<WayPoint> path = stdir.SearchShortestDrivingPath(stdir.DrivingVertex(*from), stdir.DrivingVertex(*to));
	LogOut("PATH:" <<std::endl);
	for (vector<WayPoint>::const_iterator it=path.begin(); it!=path.end(); it++) {
		if (it->type_ == WayPoint::BUS_STOP) {
			LogOut("BUS_STOP: " <<it->busStop_->xPos <<"," <<it->busStop_->yPos <<std::endl);
		} else if (it->type_ == WayPoint::ROAD_SEGMENT) {
			LogOut("ROAD_SEGMENT: " <<it->roadSegment_->getStart()->originalDB_ID.getLogItem() <<"," <<it->roadSegment_->getEnd()->originalDB_ID.getLogItem() <<std::endl);
		} else if (it->type_== WayPoint::NODE) {
			LogOut("NODE: " <<it->node_->location <<std::endl);
		} else {
			LogOut("OTHER" <<std::endl);
		}
	}



	///END DEBUG

	int lastTickPercent = 0; //So we have some idea how much time is left.
	for (unsigned int currTick = 0; currTick < config.totalRuntimeTicks; currTick++) {
		//Flag
		bool warmupDone = (currTick >= config.totalWarmupTicks);

		//Get a rough idea how far along we are
		int currTickPercent = (currTick*100)/config.totalRuntimeTicks;

		//Save the maximum number of agents at any given time
		maxAgents = std::max(maxAgents, Agent::all_agents.size());

		//Output
		if (ConfigParams::GetInstance().OutputEnabled()) {
			std::stringstream msg;
			msg << "Approximate Tick Boundary: " << currTick << ", ";
			msg << (currTick * config.baseGranMS) << " ms   [" <<currTickPercent <<"%]" << endl;
			if (!warmupDone) {
				msg << "  Warmup; output ignored." << endl;
			}
			SyncCout(msg.str());
		} else {
			//We don't need to lock this output if general output is disabled, since Agents won't
			//  perform any output (and hence there will be no contention)
			if (currTickPercent-lastTickPercent>9) {
				lastTickPercent = currTickPercent;
				cout <<currTickPercent <<"%" <<endl;
			}
		}

		///
		///  TODO: Do not delete this next line. Please read the comment in TrafficWatch.hpp
		///        ~Seth
		///
//		trafficWatch.update(currTick);

		//Agent-based cycle, steps 1,2,3,4 of 4
		WorkGroup::WaitAllGroups();

		//Check if the warmup period has ended.
		if (warmupDone) {
		}
	}

	//Finalize partition manager
	if (!config.MPI_Disabled() && config.is_run_on_many_computers) {
		PartitionManager& partitionImpl = PartitionManager::instance();
		partitionImpl.stopMPIEnvironment();
	}

	std::cout <<"Database lookup took: " <<loop_start_offset <<" ms" <<std::endl;

	cout << "Max Agents at any given time: " <<maxAgents <<std::endl;
	cout << "Starting Agents: " << numStartAgents;
	cout << ",     Pending: ";
	if (NoDynamicDispatch) {
		cout <<"<Disabled>";
	} else {
		cout <<numPendingAgents;
	}
	cout << endl;

	if (Agent::all_agents.empty()) {
		cout << "All Agents have left the simulation.\n";
	} else {
		size_t numPerson = 0;
		size_t numDriver = 0;
		size_t numPedestrian = 0;
		size_t numPassenger = 0;
		for (vector<Entity*>::iterator it = Agent::all_agents.begin(); it
				!= Agent::all_agents.end(); it++) {
			Person* p = dynamic_cast<Person*> (*it);
			if (p) {
				numPerson++;
				if (p->getRole() && dynamic_cast<Driver*> (p->getRole())) {
					numDriver++;
				}
				if (p->getRole() && dynamic_cast<Pedestrian*> (p->getRole())) {
					numPedestrian++;
				}
				if (p->getRole() && dynamic_cast<Passenger*> (p->getRole())) {
					numPassenger++;
								}
			}
		}
		cout << "Remaining Agents: " << numPerson << " (Person)   "
				<< (Agent::all_agents.size() - numPerson) << " (Other)" << endl;
		cout << "   Person Agents: " << numDriver << " (Driver)   "
				<< numPedestrian << " (Pedestrian)   " << numPassenger << " (Passenger) " << (numPerson
				- numDriver - numPedestrian) << " (Other)" << endl;
	}

	if (ConfigParams::GetInstance().numAgentsSkipped>0) {
		cout <<"Agents SKIPPED due to invalid route assignment: " <<ConfigParams::GetInstance().numAgentsSkipped <<endl;
	}

	if (!Agent::pending_agents.empty()) {
		cout << "WARNING! There are still " << Agent::pending_agents.size()
				<< " Agents waiting to be scheduled; next start time is: "
				<< Agent::pending_agents.top()->getStartTime() << " ms\n";
		if (ConfigParams::GetInstance().DynamicDispatchDisabled()) {
			throw std::runtime_error("ERROR: pending_agents shouldn't be used if Dynamic Dispatch is disabled.");
		}
	}

	//Here, we will simply scope-out the WorkGroups, and they will migrate out all remaining Agents.
	}  //End scope: WorkGroups. (Todo: should move this into its own function later)
	WorkGroup::FinalizeAllWorkGroups();

	//Test: At this point, it should be possible to delete all Signals and Agents.
	clear_delete_vector(Signal::all_signals_);
	clear_delete_vector(Agent::all_agents);

	cout << "Simulation complete; closing worker threads." << endl;
	return true;
}

int main(int argc, char* argv[])
{
	std::cout << "Using New Signal Model" << std::endl;

#if 0
	std::cout << "Not Using New Signal Model" << std::endl;
#endif
	//Save start time
	gettimeofday(&start_time, nullptr);

	/**
	 * Check whether to run SimMobility or SimMobility-MPI
	 * TODO: Retrieving ConfigParams before actually loading the config file is dangerous.
	 */
	ConfigParams& config = ConfigParams::GetInstance();
	config.is_run_on_many_computers = false;
#ifndef SIMMOB_DISABLE_MPI
	if (argc > 3 && strcmp(argv[3], "mpi") == 0) {
		config.is_run_on_many_computers = true;
	}
#endif

	/**
	 * set random be repeatable
	 * TODO: Retrieving ConfigParams before actually loading the config file is dangerous.
	 */
	config.is_simulation_repeatable = true;

	/**
	 * Start MPI if is_run_on_many_computers is true
	 */
#ifndef SIMMOB_DISABLE_MPI
	if (config.is_run_on_many_computers)
	{
		PartitionManager& partitionImpl = PartitionManager::instance();
		std::string mpi_result = partitionImpl.startMPIEnvironment(argc, argv);
		if (mpi_result.compare("") != 0)
		{
			cout << "Error:" << mpi_result << endl;
			exit(1);
		}
	}
#endif

	//Argument 1: Config file
	//Note: Don't chnage this here; change it by supplying an argument on the
	//      command line, or through Eclipse's "Run Configurations" dialog.
	std::string configFileName = "data/config.xml";
	std::string XML_OutPutFileName = "data/SimMobilityInput.xml";
	if (argc > 1)
	{
		configFileName = argv[1];
	}
	else
	{
		cout << "No config file specified; using default." << endl;
	}
	cout << "Using config file: " << configFileName << endl;

	//Argument 2: Log file
	string logFileName = argc>2 ? argv[2] : "";
	if (ConfigParams::GetInstance().OutputEnabled()) {
		if (!Logger::log_init(logFileName)) {
			cout <<"Failed to initialized log file: \"" <<logFileName <<"\"" <<", defaulting to cout." <<endl;
		}
	}

	//Perform main loop
	int returnVal = performMain(configFileName,"XML_OutPut.xml") ? 0 : 1;

	//Close log file, return.
	if (ConfigParams::GetInstance().OutputEnabled()) {
		Logger::log_done();
	}
	cout << "Done" << endl;
	return returnVal;
}

