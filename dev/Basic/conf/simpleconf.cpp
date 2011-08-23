/* Copyright Singapore-MIT Alliance for Research and Technology */

#include "simpleconf.hpp"

#include <tinyxml.h>

#include <algorithm>

//Include here (forward-declared earlier) to avoid include-cycles.
#include "../entities/Agent.hpp"
#include "../entities/Person.hpp"
#include "../entities/Region.hpp"
#include "../entities/roles/pedestrian/Pedestrian.hpp"
#include "../entities/roles/driver/Driver.hpp"
#include "../geospatial/aimsun/Loader.hpp"
#include "../geospatial/Node.hpp"
#include "../geospatial/Intersection.hpp"
#include "../geospatial/RoadSegment.hpp"
#include "../geospatial/LaneConnector.hpp"

using std::map;
using std::set;
using std::string;
using std::vector;

using namespace sim_mob;


namespace {


//Helper sort
bool agent_sort_by_id (Agent* i, Agent* j) { return (i->getId()<j->getId()); }


//Of the form xxxx,yyyyy, with optional signs
bool readPoint(const string& str, Point2D& res)
{
	//Does it match the pattern?
	size_t commaPos = str.find(',');
	if (commaPos==string::npos) {
		return false;
	}

	//Try to parse its substrings
	int xPos, yPos;
	std::istringstream(str.substr(0, commaPos)) >> xPos;
	std::istringstream(str.substr(commaPos+1, string::npos)) >> yPos;

	res = Point2D(xPos, yPos);
	return true;
}



int getValueInMS(int value, const std::string& units)
{
	//Detect errors
	if (units.empty() || (units!="minutes" && units!="seconds" && units!="ms")) {
		return -1;
	}

	//Reduce to ms
    if (units == "minutes") {
    	value *= 60*1000;
    }
    if (units == "seconds") {
    	value *= 1000;
    }

    return value;
}


int ReadGranularity(TiXmlHandle& handle, const std::string& granName)
{
	TiXmlElement* node = handle.FirstChild(granName).ToElement();
	if (!node) {
		return -1;
	}

	int value;
	const char* units = node->Attribute("units");
	if (!node->Attribute("value", &value) || !units) {
		return -1;
	}

	return getValueInMS(value, units);
}


bool loadXMLAgents(TiXmlDocument& document, std::vector<Agent*>& agents, const std::string& agentType)
{
	//Quick check.
	if (agentType!="pedestrian" && agentType!="driver") {
		return false;
	}

	//Build the expression dynamically.
	TiXmlHandle handle(&document);
	TiXmlElement* node = handle.FirstChild("config").FirstChild(agentType+"s").FirstChild(agentType).ToElement();
	if (!node) {
		//Agents are optional
		return true;
	}

	//Loop through all agents of this type
	for (;node;node=node->NextSiblingElement()) {
		//xmlNode* curr = xpObject->nodesetval->nodeTab[i];
		Person* agent = nullptr;
		bool foundID = false;
		bool foundXPos = false;
		bool foundYPos = false;
		bool foundOrigPos = false;
		bool foundDestPos = false;
		for (TiXmlAttribute* attr=node->FirstAttribute(); attr; attr=attr->Next()) {
			//Read each attribute.
			std::string name = attr->NameTStr();
			std::string value = attr->ValueStr();
			if (name.empty() || value.empty()) {
				return false;
			}
			int valueI=-1;
			if (name=="id" || name=="xPos" || name=="yPos"||name=="time") {
				std::istringstream(value) >> valueI;
			}

			//Assign it.
			if (name=="id") {
				agent = new Person(valueI);
				if (agentType=="pedestrian") {
					agent->changeRole(new Pedestrian(agent));
				} else if (agentType=="driver") {
					agent->changeRole(new Driver(agent));
				}
				foundID = true;
			} else if (name=="xPos") {
				agent->xPos.force(valueI);
				foundXPos = true;
			} else if (name=="yPos") {
				agent->yPos.force(valueI);
				foundYPos = true;
			} else if (name=="originPos") {
				Point2D pt;
				if (!readPoint(value, pt)) {
					std::cout <<"Couldn't read point from value: " <<value <<"\n";
					return false;
				}
				agent->originNode = ConfigParams::GetInstance().getNetwork().locateNode(pt, true);
				if (!agent->originNode) {
					std::cout <<"Couldn't find position: " <<pt.getX() <<"," <<pt.getY() <<"\n";
					return false;
				}
				foundOrigPos = true;
			} else if (name=="destPos") {
				Point2D pt;
				if (!readPoint(value, pt)) {
					std::cout <<"Couldn't read point from value: " <<value <<"\n";
					return false;
				}
				agent->destNode = ConfigParams::GetInstance().getNetwork().locateNode(pt, true);
				if (!agent->destNode) {
					std::cout <<"Couldn't find position: " <<pt.getX() <<"," <<pt.getY() <<"\n";
					return false;
				}
				foundDestPos = true;
			}
			else if (name=="time"){
				agent->startTime=valueI;
			}
			else {
				return false;
			}
		}

		//Simple checks
		bool foundOldPos = foundXPos && foundYPos;
		if (!foundID) {
			std::cout <<"id attribute not found.\n";
			return false;
		}
		if (!foundOldPos && !foundOrigPos && !foundDestPos) {
			std::cout <<"agent position information not found.\n";
			return false;
		}

		//Slightly more complex checks
		if (foundOldPos && (foundOrigPos || foundDestPos)) {
			std::cout <<"agent contains both old and new-style position information.\n";
			return false;
		}


		//Save it.
		agents.push_back(agent);
	}

	return true;
}



bool LoadDatabaseDetails(TiXmlElement& parentElem, string& connectionString, map<string, string>& storedProcedures)
{
	TiXmlHandle handle(&parentElem);
	TiXmlElement* elem = handle.FirstChild("connection").FirstChild("param").ToElement();
	if (!elem) {
		return false;
	}

	//Loop through each parameter; add it to the connection string
	for (;elem;elem=elem->NextSiblingElement()) {
		const char* name = elem->Attribute("name");
		const char* value = elem->Attribute("value");
		if (!name || !value) {
			return false;
		}
		string pair = (connectionString.empty()?"":" ") + string(name) + "=" + string(value);
		connectionString += pair;
	}

	//Now, load the stored procedure mappings
	elem = handle.FirstChild("mappings").FirstChild().ToElement();
	if (!elem) {
		return false;
	}

	//Loop through and add them.
	for (;elem;elem=elem->NextSiblingElement()) {
		string name = elem->ValueStr();
		const char* value = elem->Attribute("procedure");
		if (!value) {
			return false;
		}
		if (storedProcedures.count(name)!=0) {
			return false;
		}

		storedProcedures[name] = string(value);
	}

	//Done; we'll check the storedProcedures in detail later.
	return !connectionString.empty() && storedProcedures.size()==4;
}



bool LoadXMLBoundariesCrossings(TiXmlDocument& document, const string& parentStr, const string& childStr, map<string, Point2D>& result)
{
	//Quick check.
	if (parentStr!="boundaries" && parentStr!="crossings") {
		return false;
	}
	if (childStr!="boundary" && childStr!="crossing") {
		return false;
	}

	//Build the expression dynamically.
	TiXmlHandle handle(&document);
	TiXmlElement* node = handle.FirstChild("config").FirstChild(parentStr).FirstChild(childStr).ToElement();
	if (!node) {
		//Boundaries/crossings are optional
		return true;
	}

	//Move through results
	for (;node; node=node->NextSiblingElement()) {
		//xmlNode* curr = xpObject->nodesetval->nodeTab[i];
		string key;
		double xPos;
		double yPos;
		unsigned int flagCheck = 0;
		for (TiXmlAttribute* attr=node->FirstAttribute(); attr; attr=attr->Next()) {
			//Read each attribute.
			std::string name = attr->NameTStr();
			std::string value = attr->ValueStr();
			if (name.empty() || value.empty()) {
				return false;
			}

			//Assign it.
			if (name=="position") {
				key = value;
				flagCheck |= 1;
			} else {
				int valueI;
				std::istringstream(value) >> valueI;
				if (name=="xPos") {
					xPos = valueI;
					flagCheck |= 2;
				} else if (name=="yPos") {
					yPos = valueI;
					flagCheck |= 4;
				} else {
					return false;
				}
			}
		}

		if (flagCheck!=7) {
			return false;
		}

		//Save it.
		result[key] = Point2D(xPos, yPos);
	}

	return true;
}


void ensureID(map<RoadSegment*, int>& segIDs, map<int, RoadSegment*>& revSegIDs, RoadSegment* item)
{
	if (segIDs.count(item)>0) {
		return;
	}

	int newID = segIDs.size();
	segIDs[item] = newID;
	revSegIDs[newID] = item;

	std::cout <<"Adding new Segment ID: " <<newID <<" with item: " << item <<"\n";
}


void PrintDB_Network()
{
	//Road Segments need a temporary ID value (for display)
	map<RoadSegment*, int> segIDs;
	map<int, RoadSegment*> revSegIDs;

	//Start by printing nodes.
	RoadNetwork& rn = ConfigParams::GetInstance().getNetwork();
	for (vector<MultiNode*>::const_iterator it=rn.getNodes().begin(); it!=rn.getNodes().end(); it++) {
		std::streamsize oldSz = std::cout.precision();
		std::cout.precision(10);
		std::cout <<"Node: " <<(*it)->location->getX() <<"," <<(*it)->location->getY() <<"\n";
		std::cout.precision(oldSz);

		//NOTE: This is temporary; later we'll ensure that the RoadNetwork only stores Intersections,
		//      and RoadSegments will have to be extracted.
		const Intersection* nodeInt = static_cast<const Intersection*>((*it));

		//Print all segments
		for (set<RoadSegment*>::const_iterator i2=nodeInt->getRoadSegments().begin(); i2!=nodeInt->getRoadSegments().end(); ++i2) {
			ensureID(segIDs, revSegIDs, *i2);
			std::cout <<"   Has segement: " <<segIDs[*i2] <<" for memory location " <<*i2 <<"\n";
		}

		//Now print all lane connectors at this node
		for (set<RoadSegment*>::iterator rsIt=nodeInt->getRoadSegments().begin(); rsIt!=nodeInt->getRoadSegments().end(); rsIt++) {
			if (nodeInt->hasOutgoingLanes(**rsIt)) {
				for (set<LaneConnector*>::iterator i2=nodeInt->getOutgoingLanes(**rsIt).begin(); i2!=nodeInt->getOutgoingLanes(**rsIt).end(); i2++) {
					//For now, just assigning a temporary segment id.
					RoadSegment* fromSeg = (*i2)->getLaneFrom()->getRoadSegment();
					unsigned int fromLane = (*i2)->getLaneFrom()->getLaneID();
					RoadSegment* toSeg = (*i2)->getLaneTo()->getRoadSegment();
					unsigned int toLane = (*i2)->getLaneTo()->getLaneID();

					//Make sure they have IDs
					ensureID(segIDs, revSegIDs, fromSeg);
					ensureID(segIDs, revSegIDs, toSeg);

					//Output
					std::cout <<"    Connector, from segment " <<segIDs[fromSeg] <<", lane " <<fromLane <<"  to segment " <<segIDs[toSeg] <<", lane " <<toLane <<"\n";
				}
			}
		}
	}

	//Now print all Segments
	for (size_t i=0; i<segIDs.size(); i++) {
		RoadSegment* seg = revSegIDs[i];
		if (!seg) {
			std::cout <<"Error: No segment lookup (or is null) for ID: " <<i <<std::endl;
		}
		std::cout <<"Segment[" <<i <<"]  name: " <<seg->getLink()->roadName;
		std::streamsize oldSz = std::cout.precision();
		std::cout.precision(10);
		std::cout <<", length: " <<seg->length;
		std::cout.precision(oldSz);
		std::cout <<", speed: " <<seg->maxSpeed;
		std::cout <<", width: " <<seg->width;
		std::cout <<", lanes: " <<seg->getLanes().size() <<"\n";

		if (!seg->polyline.empty()) {
			std::cout <<"    Polyline: ";
			for (vector<Point2D>::const_iterator it=seg->polyline.begin(); it!=seg->polyline.end(); it++) {
				std::streamsize oldSz = std::cout.precision();
				std::cout.precision(10);
				std::cout <<"(" <<it->getX() <<"," <<it->getY() <<") ";
				std::cout.precision(oldSz);
			}
			std::cout <<"\n";
		}
	}

	//TODO: Print links and RoadSegmentNodes in a sensible fashion.
}



//Returns the error message, or an empty string if no error.
std::string loadXMLConf(TiXmlDocument& document, std::vector<Agent*>& agents)
{
	//Save granularities: system
	TiXmlHandle handle(&document);
	handle = handle.FirstChild("config").FirstChild("system").FirstChild("simulation");
	int baseGran = ReadGranularity(handle, "base_granularity");
	int totalRuntime = ReadGranularity(handle, "total_runtime");
	int totalWarmup = ReadGranularity(handle, "total_warmup");

	//Save more granularities
	handle = TiXmlHandle(&document);
	handle = handle.FirstChild("config").FirstChild("system").FirstChild("granularities");
	int granAgent = ReadGranularity(handle, "agent");
	int granSignal = ReadGranularity(handle, "signal");
	int granPaths = ReadGranularity(handle, "paths");
	int granDecomp = ReadGranularity(handle, "decomp");

	//Check
    if(    baseGran==-1 || totalRuntime==-1 || totalWarmup==-1
    	|| granAgent==-1 || granSignal==-1 || granPaths==-1 || granDecomp==-1) {
        return "Unable to read config file.";
    }

    //Granularity check
    if (granAgent%baseGran != 0) {
    	return "Agent granularity not a multiple of base granularity.";
    }
    if (granSignal%baseGran != 0) {
    	return "Signal granularity not a multiple of base granularity.";
    }
    if (granPaths%baseGran != 0) {
    	return "Path granularity not a multiple of base granularity.";
    }
    if (granDecomp%baseGran != 0) {
    	return "Decomposition granularity not a multiple of base granularity.";
    }
    if (totalRuntime%baseGran != 0) {
    	std::cout <<"  Warning! Total Runtime will be truncated.\n";
    }
    if (totalWarmup%baseGran != 0) {
    	std::cout <<"  Warning! Total Warmup will be truncated.\n";
    }

    //Save params
    {
    	ConfigParams& config = ConfigParams::GetInstance();
    	config.baseGranMS = baseGran;
    	config.totalRuntimeTicks = totalRuntime/baseGran;
    	config.totalWarmupTicks = totalWarmup/baseGran;
    	config.granAgentsTicks = granAgent/baseGran;
    	config.granSignalsTicks = granSignal/baseGran;
    	config.granPathsTicks = granPaths/baseGran;
    	config.granDecompTicks = granDecomp/baseGran;
    }


    //Check the type of geometry
    handle = TiXmlHandle(&document);
    TiXmlElement* geomElem = handle.FirstChild("config").FirstChild("geometry").ToElement();
    if (geomElem) {
    	const char* geomType = geomElem->Attribute("type");
    	if (geomType && string(geomType) == "simple") {
    		//Load boundaries
    		if (!LoadXMLBoundariesCrossings(document, "boundaries", "boundary", ConfigParams::GetInstance().boundaries)) {
    			return "Couldn't load boundaries";
    		}

    		//Load crossings
    		if (!LoadXMLBoundariesCrossings(document, "crossings", "crossing", ConfigParams::GetInstance().crossings)) {
    			return "Couldn't load crossings";
    		}
    	} else if (geomType && string(geomType) == "aimsun") {
    		//Ensure we're loading from a database
    		const char* geomSrc = geomElem->Attribute("source");
    		if (!geomSrc || "database" != string(geomSrc)) {
    			return "Unknown geometry source: " + (geomSrc?string(geomSrc):"");
    		}

    		//Load the AIMSUM network details
    		map<string, string> storedProcedures; //Of the form "node" -> "get_node()"
    		if (!LoadDatabaseDetails(*geomElem, ConfigParams::GetInstance().connectionString, storedProcedures)) {
    			return "Unable to load database connection settings.";
    		}

    		//Confirm that all stored procedures have been set.
    		if (
    			   storedProcedures.count("node")==0 || storedProcedures.count("section")==0
    			|| storedProcedures.count("turning")==0 || storedProcedures.count("polyline")==0
    		) {
    			return "Not all stored procedures were specified.";
    		}

    		//Actually load it
    		string dbErrorMsg = sim_mob::aimsun::Loader::LoadNetwork(ConfigParams::GetInstance().connectionString, storedProcedures, ConfigParams::GetInstance().getNetwork());
    		if (!dbErrorMsg.empty()) {
    			return "Database loading error: " + dbErrorMsg;
    		}

    		//Finally, mask the password
    		string& s = ConfigParams::GetInstance().connectionString;
    		size_t check = s.find("password=");
    		if (check!=string::npos) {
    			size_t start = s.find("=", check) + 1;
    			size_t end = s.find(" ", start);
    			size_t amt = ((end==string::npos) ? s.size() : end) - start;
    			s = s.replace(start, amt, amt, '*');
    		}
    	} else {
    		return "Unknown geometry type: " + (geomType?string(geomType):"");
    	}
    }


    //Load ALL agents: drivers and pedestrians.
    //  (Note: Use separate config files if you just want to test one kind of agent.)
    if (!loadXMLAgents(document, agents, "pedestrian")) {
    	return "Couldn't load pedestrians";
    }
    if (!loadXMLAgents(document, agents, "driver")) {
    	return	 "Couldn't load drivers";
    }

    //Sort agents by id.
    //TEMP: Eventually, we'll need a more sane way to deal with agent IDs.
    std::sort(agents.begin(), agents.end(), agent_sort_by_id);


    //Display
    std::cout <<"Config parameters:\n";
    std::cout <<"------------------\n";
    std::cout <<"  Base Granularity: " <<ConfigParams::GetInstance().baseGranMS <<" " <<"ms" <<"\n";
    std::cout <<"  Total Runtime: " <<ConfigParams::GetInstance().totalRuntimeTicks <<" " <<"ticks" <<"\n";
    std::cout <<"  Total Warmup: " <<ConfigParams::GetInstance().totalWarmupTicks <<" " <<"ticks" <<"\n";
    std::cout <<"  Agent Granularity: " <<ConfigParams::GetInstance().granAgentsTicks <<" " <<"ticks" <<"\n";
    std::cout <<"  Signal Granularity: " <<ConfigParams::GetInstance().granSignalsTicks <<" " <<"ticks" <<"\n";
    std::cout <<"  Paths Granularity: " <<ConfigParams::GetInstance().granPathsTicks <<" " <<"ticks" <<"\n";
    std::cout <<"  Decomp Granularity: " <<ConfigParams::GetInstance().granDecompTicks <<" " <<"ticks" <<"\n";
    if (!ConfigParams::GetInstance().boundaries.empty() || !ConfigParams::GetInstance().crossings.empty()) {
    	std::cout <<"  Boundaries Found: " <<ConfigParams::GetInstance().boundaries.size() <<"\n";
		for (map<string, Point2D>::iterator it=ConfigParams::GetInstance().boundaries.begin(); it!=ConfigParams::GetInstance().boundaries.end(); it++) {
			std::cout <<"    Boundary[" <<it->first <<"] = (" <<it->second.getX() <<"," <<it->second.getY() <<")\n";
		}
		std::cout <<"  Crossings Found: " <<ConfigParams::GetInstance().crossings.size() <<"\n";
		for (map<string, Point2D>::iterator it=ConfigParams::GetInstance().crossings.begin(); it!=ConfigParams::GetInstance().crossings.end(); it++) {
			std::cout <<"    Crossing[" <<it->first <<"] = (" <<it->second.getX() <<"," <<it->second.getY() <<")\n";
		}
    }
    if (!ConfigParams::GetInstance().connectionString.empty()) {
    	//Output AIMSUN data
    	std::cout <<"Network details loaded from connection: " <<ConfigParams::GetInstance().connectionString <<"\n";
    	std::cout <<"------------------\n";
    	PrintDB_Network();
    	std::cout <<"------------------\n";
    }
    std::cout <<"  Agents Initialized: " <<agents.size() <<"\n";
    for (size_t i=0; i<agents.size(); i++) {
    	std::cout <<"    Agent(" <<agents[i]->getId() <<") = " <<agents[i]->xPos.get() <<"," <<agents[i]->yPos.get() <<"\n";
    }
    std::cout <<"------------------\n";

	//No error
	return "";
}



} //End anon namespace



//////////////////////////////////////////
// Simple singleton implementation
//////////////////////////////////////////
ConfigParams sim_mob::ConfigParams::instance;
sim_mob::ConfigParams::ConfigParams() {

}
ConfigParams& sim_mob::ConfigParams::GetInstance() {
	return ConfigParams::instance;
}





//////////////////////////////////////////
// Main external method
//////////////////////////////////////////

bool sim_mob::ConfigParams::InitUserConf(const string& configPath, std::vector<Agent*>& agents, std::vector<Region*>& regions,
		          std::vector<TripChain*>& trips, std::vector<ChoiceSet*>& chSets,
		          std::vector<Vehicle*>& vehicles)
{
	//Load our config file into an XML document object.
	TiXmlDocument doc(configPath);
	if (!doc.LoadFile()) {
		std::cout <<"Error loading config file: " <<doc.ErrorDesc() <<std::endl;
		return false;
	}

	//Parse it
	string errorMsg = loadXMLConf(doc, agents);
	if (errorMsg.empty()) {
		std::cout <<"XML config file loaded." <<std::endl;
	} else {
		std::cout <<"Aborting on Config Error: " <<errorMsg <<std::endl;
	}

	//TEMP:
	for (size_t i=0; i<5; i++)
		regions.push_back(new Region(i));
	for (size_t i=0; i<6; i++)
		trips.push_back(new TripChain(i));
	for (size_t i=0; i<15; i++)
		chSets.push_back(new ChoiceSet(i));
	for (size_t i=0; i<10; i++)
		vehicles.push_back(new Vehicle(i));
	if (errorMsg.empty()) {
		std::cout <<"Configuration complete." <<std::endl;
	}


	return errorMsg.empty();

}
