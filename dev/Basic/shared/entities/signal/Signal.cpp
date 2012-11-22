/* Copyright Singapore-MIT Alliance for Research and Technology */

/*
 * Signal.cpp
 *
 *  Created on: 2011-7-18
 *      Author: Vahid Saber
 */

#include "Signal.hpp"
#ifdef SIMMOB_NEW_SIGNAL

#include <math.h>
#include "geospatial/Lane.hpp"
#include "geospatial/Crossing.hpp"
#include "geospatial/MultiNode.hpp"
#include "geospatial/RoadSegment.hpp"
#include "geospatial/streetdir/StreetDirectory.hpp"
#include "util/OutputUtil.hpp"
#include "conf/simpleconf.hpp"

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/mem_fun.hpp>


#ifndef SIMMOB_DISABLE_MPI
#include "partitions/PackageUtils.hpp"
#include "partitions/UnPackageUtils.hpp"
#endif

using std::map;
using std::vector;
using std::string;

using namespace boost::multi_index;
using boost::multi_index::get;

typedef sim_mob::Entity::UpdateStatus UpdateStatus;

sim_mob::Signal::All_Signals sim_mob::Signal::all_signals_;

namespace sim_mob
{

Signal_SCATS const &
Signal_SCATS::signalAt(Node const & node, const MutexStrategy& mtxStrat, bool *isNew ) {
	if (isNew) { *isNew = false; }
	Signal_SCATS const * signal = dynamic_cast<Signal_SCATS const *>(StreetDirectory::instance().signalAt(node));
	if (signal)
	{
		return *signal;
	}

	Signal_SCATS * sig = new Signal_SCATS(node, mtxStrat);
	all_signals_.push_back(sig);
	if (isNew) { *isNew = true; }
	StreetDirectory::instance().registerSignal(*sig);
//	std::cout << "Signal Created\n";
	return *sig;
}
std::string Signal_SCATS::toString() const { return strRepr; }
unsigned int Signal_SCATS::getSignalId()   {return signalID;}
unsigned int Signal_SCATS::getSignalId() const  {return signalID;}
bool Signal_SCATS::isIntersection() { return isIntersection_;}

void Signal_SCATS::createStringRepresentation(std::string newLine)
{
	std::ostringstream output;
			output << "{" << newLine << "\"TrafficSignal\":" << "{" << newLine;
			output << "\"hex_id\":\""<< this << "\"," << newLine;
			output << "\"frame\": " << -1 << "," << newLine; //this is added to indicate that
			output << "\"simmob_id\":\"" <<  signalID << "\"," << newLine;
			output << "\"node\": \"" << &getNode() << "\"," << newLine;
//			output << plan_.createStringRepresentation(newLine); 2nd time
			output << "\"phases\":" << newLine << "[";
			for(int i = 0; i < getPhases().size(); i++)
			{
				output << ( getPhases()[i]).createStringRepresentation(newLine);
				if((i + 1) < getPhases().size()) output << ",";
			}
//				while(it !=getPhases().end())
//				{
//				}
				output << newLine << "]";
			output  << newLine << "}"  << newLine << "}";
			strRepr = output.str();//all the aim of the unrelated part
}

void
Signal_SCATS::printColors(double currCycleTimer)
{
	phases_iterator it = getPhases().begin();
	while(it !=getPhases().end())
	{
		(*it).printPhaseColors(currCycleTimer);
		it++;
	}
}

/*Signal Constructor*/
Signal_SCATS::Signal_SCATS(Node const & node, const MutexStrategy& mtxStrat, int id, signalType type_)
  :  loopDetector_(new LoopDetectorEntity(*this, mtxStrat))
	,Signal(node,mtxStrat,id)
	/*, node_(node)*/
{
	setSignalType(type_);
	const MultiNode* mNode = dynamic_cast<const MultiNode*>(&getNode());
	if(! mNode) isIntersection_ = false ;
	else isIntersection_ = true;
	//some inits
	currCycleTimer = 0;
	currPhaseID = 0;
	isNewCycle = false;
	currOffset = 0;
	//the best id for a signal is the node id itself
	signalID = node.getID();

//	findSignalLinksAndCrossings(); todo:eoved temporarily

	//for future use when user needs to switch between fixed and adaptive control
	signalTimingMode = ConfigParams::GetInstance().signalTimingMode;
//	findIncomingLanes();//what was it used for? only Density?
	//it would be better to declare it as static const
	updateInterval = sim_mob::ConfigParams::GetInstance().granSignalsTicks * sim_mob::ConfigParams::GetInstance().baseGranMS / 1000;
	currCycleTimer = 0;
//    setupIndexMaps();  I guess this function is Not needed any more
}

// Return the Crossing object, if any, in the specified road segment.  If there are more
// than one Crossing objects, return the one that has the least offset.
Crossing const *
Signal_SCATS::getCrossing(RoadSegment const * road) {
	//Crossing const * result = 0;
	//double offset = std::numeric_limits<double>::max();
	int currOffset = 0;
	int minus = 1;
	int i;
	if(road->getStart() == &(this->getNode()))
	{
		currOffset = 0;
		minus = 1;//increment offset
	}
	else
	{
		currOffset = road->length;
//		minus = -1;//decrement offset
	}
	for (i =0;;i++) {
		//Get the next item, if any.
		RoadItemAndOffsetPair res = road->nextObstacle(currOffset, true);
		if (!res.item) {
			std::cout << "breaking after " << i <<" iterations " << std::endl;
			break;
		}

		//Check if it's a Crossing.
		if (Crossing const * crossing = dynamic_cast<Crossing const *>(res.item)) {
			if(getNode().getID()== 115436)
			{
				std::cout << "Crossing for node 115436 found at offset " << currOffset << "minus is : " << minus << std::endl;
//				//getchar();
			}
			//Success
			return crossing;
		}

		//Increment OR Decrement
		currOffset += (minus) * (res.offset);
	}
		std::cout << "No Crossing for this segment of node 115436 found after " << i <<" iterations minus was : " << minus << std::endl;

	//Failure.
	return nullptr;
}

// This functor calculates the angle between a link and a reference link, which have a node
// in common.
struct AngleCalculator {
	// <node> is the node in common and <refLink> is the reference link.  Therefore <node>
	// must be one of the ends of <refLink>.
	AngleCalculator(Node const & node, Link const * refLink) :
		center_(node) {
		assert(refLink->getStart() == &center_ || refLink->getEnd() == &center_);
		// <refAngle_> is the angle that the <refLink> makes with the X-axis.
		refAngle_ = angle(refLink);
	}

	// Calculates the angle between <link> and <refLink>, which have <node> in common.
	// Therefore <node> must be one of the ends of <link>.
	double operator()(Link const * link) const {
		assert(link->getStart() == &center_ || link->getEnd() == &center_);
		return angle(link) - refAngle_;
	}

	Node const & center_;
	double refAngle_;

	// Caculates the angle that <link> with respect to the X-axis.
	double angle(Link const * link) const {
		Point2D point;
		if (link->getStart() == &center_)
			point = link->getEnd()->location;
		else
			point = link->getStart()->location;
		double xDiff = point.getX() - center_.location.getX();
		double yDiff = point.getY() - center_.location.getY();
		return atan2(yDiff, xDiff);
	}
};


/*
 * this class needs to access lanes coming to it, mostly to calculate DS
 * It is not feasible to extract the lanes from every traffic signal every time
 * we need to calculate DS. Rather, we book-keep  the lane information.
 * It is a trade-off between process and memory.
 * In order to save memory, we only keep the record of Lane pointers-vahid
 */
void Signal_SCATS::findSignalLinksAndCrossings() {
	std::pair<LinkAndCrossingByLink::iterator, bool> p;
	LinkAndCrossingByLink & inserter = get<2>(getLinkAndCrossing()); //2 means that duplicate links will not be allowed
	const MultiNode* mNode = dynamic_cast<const MultiNode*>(&getNode());
	if (!mNode)
		return;
	const std::set<sim_mob::RoadSegment*>& roads = mNode->getRoadSegments();
	std::set<RoadSegment*>::const_iterator iter = roads.begin();
	sim_mob::RoadSegment const * road = *iter;
	sim_mob::Crossing const * crossing = getCrossing(road);
	sim_mob::Link const * link = road->getLink();
	p = inserter.insert(LinkAndCrossing(0, link, crossing, 0));
	++iter;


	AngleCalculator angle(getNode(), link);
	double angleAngle = 0;
	size_t id = 1;
	angleAngle = 0;
	crossing = nullptr;
	link = nullptr;
	for (; iter != roads.end(); ++iter, ++id) { //id=1 coz we have already made an insertion with id=0 above
		road = *iter;
		crossing = getCrossing(road);
		link = road->getLink();
		angleAngle = angle.angle(link);

		p = inserter.insert(LinkAndCrossing(id, link, crossing, angleAngle));
		crossing = nullptr;
		link = nullptr;
	}
}

//find the minimum among the max projected DS
int Signal_SCATS::fmin_ID(const std::vector<double> maxproDS) {
	int min = 0;
	for (int i = 1; i < maxproDS.size(); i++) {
		if (maxproDS[i] < maxproDS[min]) {
			min = i;
		}
		//else{}
	}
	return min;
}

//This function will calculate the DS at the end of each phase considering only the max DS of lane in the LinkFrom(s)
//LinkFrom(s) are the links from which vehicles enter the intersection during the corresponding phase
double Signal_SCATS::computePhaseDS(int phaseId) {
	double lane_DS = 0, maxPhaseDS = 0, maxDS = 0;
	sim_mob::Phase p_it = getPhases()[phaseId];

	double total_g = p_it.computeTotalG(); //todo: I guess we can avoid calling this function EVERY time by adding an extra container at split plan level.(mapped to choiceSet container)
	sim_mob::Phase::links_map_iterator link_it = (p_it).LinkFrom_begin();
	for (; link_it != (p_it).LinkFrom_end(); link_it++) { //Loop2===>link
		std::set<sim_mob::RoadSegment*> segments =
				(*link_it).first->getUniqueSegments(); //optimization: use either fwd or bed segments
		std::set<sim_mob::RoadSegment*>::iterator seg_it = segments.begin();
		for (; seg_it != segments.end(); seg_it++) { //Loop3===>road segment
			//discard the segments that don't end here(coz those who don't end here, don't cross the intersection neither)
			//sim_mob::Link is bi-directionl so we use RoadSegment's start and end to imply direction
			if ((*seg_it)->getEnd() != &getNode())
				continue;
			const std::vector<sim_mob::Lane*> lanes = (*seg_it)->getLanes();
			for (std::size_t i = 0; i < lanes.size(); i++) { //Loop4===>lane
				const Lane* lane = nullptr;
				lane = lanes.at(i);
				if (lane->is_pedestrian_lane())
					continue;
				const LoopDetectorEntity::CountAndTimePair& ctPair =
						loopDetector_->getCountAndTimePair(*lane);
				lane_DS = LaneDS(ctPair, total_g);
//				std::cout << "lane_DS = " << lane_DS << std::endl;
				if (lane_DS > maxPhaseDS)
					maxPhaseDS = lane_DS;
			}
		}
	}

	Phase_Density[phaseId] = maxPhaseDS;
	loopDetector_->reset();
	return Phase_Density[phaseId];
}

/*
 * The actual DS computation formula is here!
 * It calculates the DS on a specific Lane
 * at the moment total_g amounts to total_g at each phase,
 * However this function doesn't care total_g comes from which scop(phase level, cycle level....)
 */
double Signal_SCATS::LaneDS(const LoopDetectorEntity::CountAndTimePair& ctPair,double total_g)
{
//	CountAndTimePair would give you T and n of the formula 2 in section 3.2 of the memurandum (page 3)
//	std::cout << "ctPair(vehicle count: " << ctPair.vehicleCount << " , spaceTime: " << ctPair.spaceTimeInMilliSeconds << ")"
//			<< " total_g=" << total_g
//			<< std::endl;
	std::size_t vehicleCount = ctPair.vehicleCount;
	unsigned int spaceTime = ctPair.spaceTimeInMilliSeconds;
	double standard_space_time = 1.04*1000;//1.04 seconds
	/*this is formula 2 in section 3.2 of the memurandum (page 3)*/
	double used_g = (vehicleCount==0)?0:total_g - (spaceTime - standard_space_time*vehicleCount);
	return used_g/total_g;//And this is formula 1 in section 3.2 of the memurandum (page 3)
}
void Signal_SCATS::cycle_reset()
{
	loopDetector_->reset();//extra
	isNewCycle = false;
//	DS_all = 0;
	for(int i = 0; i < Phase_Density.size(); Phase_Density[i++] = 0);
}

//This is a part of Signal_SCATS::update function that is executed only if a new cycle has reached
void Signal_SCATS::newCycleUpdate()
{
	std::cout << "Inside newCycleUpdate \n";

	//	6-update split plan
		plan_.Update(Phase_Density);
	//	7-update offset
//		offset_.update(cycle_.getnextCL());
		cycle_reset();
		loopDetector_->reset();//extra
		isNewCycle = false;
}

bool Signal_SCATS::updateCurrCycleTimer() {
	bool is_NewCycle = false;
	if((currCycleTimer + updateInterval) >= plan_.getCycleLength())
		{
			is_NewCycle = true;
		}
	//even if it is a new cycle(and a ew cycle length, the currCycleTimer will hold the amount of time system has proceeded to the new cycle
	currCycleTimer =  std::fmod((currCycleTimer + updateInterval) , plan_.getCycleLength());
	return is_NewCycle;
}

//Output To Visualizer
void Signal_SCATS::outputTrafficLights(frame_t frameNumber,std::string newLine) {
	std::stringstream output;
	output << newLine << "{" << newLine << "\"TrafficSignalUpdate\":" << newLine <<"{" << newLine ;
	output << "\"hex_id\":\""<< this << "\"," << newLine;
	output << "\"frame\": " << frameNumber << "," << newLine;
	//phase.......
//	output << plan_.outputTrafficLights(newLine);
	if(getNOF_Phases() == 0)
		{
			LogOut( output.str() << newLine << "}" << newLine << "}" << std::endl);
			return;
		}
//	std::ostringstream output;
	output << "\"currPhase\": \"" << getPhases()[currPhaseID].getName() << "\"," << newLine;
//	std::cout << "currPhase " << getPhases()[currPhaseID].getName();
	output << "\"phases\":" << newLine << "[";

	//////////////////////////////////////////////////
	for(int i =0; i < getPhases().size(); i++)
	{
		output << getPhases()[i].outputPhaseTrafficLight(newLine);
		if((i + 1) < getPhases().size()) output << ",";
	}
	output << newLine << "]";
//	getchar();
//	/////////////////////////////////////////////////////
//
//	while(it !=getPhases().end())
//	{
//		std::cout << "getting the out put rep of phase " << (*it).getName();
//		getchar();
//		output << (*it).outputPhaseTrafficLight(newLine);
//		it++;
//		if(it !=getPhases().end())
//			output << ",";
//	}
//	output << newLine << "]";
//	return output.str();
	//........
//	std::cout << "Outputting " << output.str() << std::endl;
//	getchar();
	LogOut( output.str() << newLine << "}" << newLine << "}" << std::endl);
}
std::size_t Signal_SCATS::computeCurrPhase(double currCycleTimer)
{
	std::vector< double > currSplitPlan = plan_.CurrSplitPlan();

	double sum = 0;
	int i;
	for(i = 0; i < getNOF_Phases(); i++)
	{
		//expanded the single line loop, for better understanding of future readers
		sum += plan_.getCycleLength() * currSplitPlan[i] / 100;
		if(sum > currCycleTimer) break;
	}
//	  std::cout << "Signal " << sim_mob::Signal::all_signals_.front()->getId() << "  Has " <<  sim_mob::Signal::all_signals_.front()->getPhases().size() << " phases\n"; getchar();

	if(i >= getNOF_Phases())
		{
			std::stringstream str;
			str << "Signal " << this->getId() << " CouldNot computeCurrPhase for the given currCycleTimer(" << currCycleTimer <<  "/" << plan_.getCycleLength() << ") getNOF_Phases()(" <<  getNOF_Phases() << ") , sum of cycleLength chunks(" << sum << ")";
//			 +currCycleTimer+") getNOF_Phases()("+ getNOF_Phases() +"sum of cycleLength chunks(" + sum+")"
			throw std::runtime_error(str.str());
		}
	currPhaseID = (std::size_t)(i);
	return currPhaseID;
}
/*
 * 1- update current cycle timer
 * 2- update current phase color
 * 3- update current phase
 * if current cycle timer indicates end of cycle:
 * 4-compute DS
 * 5-update cycle length
 * 6-update split plan
 * 7-update offset
 * end of if
 * 8-reset the loop detector to make it ready for the next cycle
 * 8-start
 */
UpdateStatus Signal_SCATS::update(frame_t frameNumber) {
	if(!isIntersection_) return UpdateStatus::Continue;
	isNewCycle = false;
	outputTrafficLights(frameNumber,"");
//	1- update current cycle timer( Signal_SCATS::currCycleTimer)
	isNewCycle = updateCurrCycleTimer();
	//if the phase has changed, here we dont update currPhaseID to a new value coz we still need some info(like DS) obtained during the last phase
	//	3-Update Current Phase
	int temp_PhaseId = computeCurrPhase(currCycleTimer);

//	2- update current phase color
	if(temp_PhaseId < getNOF_Phases())
		{
			getPhases()[temp_PhaseId].update(currCycleTimer);
		}
	else
		throw std::runtime_error("currPhaseID out of range");

	if((currPhaseID != temp_PhaseId) && signalTimingMode)//separated coz we may need to transfer computeDS here
		{
			computePhaseDS(currPhaseID);
			currPhaseID  = temp_PhaseId;
		}
	if(isNewCycle && signalTimingMode)
	{
		newCycleUpdate();
		initializePhases();
	}

//	outputToVisualizer(frameNumber);
//Not mine, don't know much about what benefit it has to the outside world
	return UpdateStatus::Continue;
}

/*I will try to change only the Data structure, not the algorithm-vahid
 * This function will tell you what lights a driver is gonna get when he is at the traffic signal
 * this is done based on the lan->rs->link he is in.
 * He will get three colors for three options of heading directions(left, forward,right)
 */

TrafficColor Signal_SCATS::getDriverLight(Lane const & fromLane, Lane const & toLane) const {
	RoadSegment const * fromRoad = fromLane.getRoadSegment();
	Link * const fromLink = fromRoad->getLink();

	RoadSegment const * toRoad = toLane.getRoadSegment();
	Link const * toLink = toRoad->getLink();

	const sim_mob::Phase &currPhase = getCurrPhase();
	sim_mob::Phase::links_map_equal_range range = currPhase.getLinkTos(fromLink);
	sim_mob::Phase::links_map_const_iterator iter;
	for(iter = range.first; iter != range.second ; iter++ )
	{
		if((*iter).second.LinkTo == toLink)
			break;
	}

	//if the link is not listed in the current phase throw an error (alternatively, just return red)
	if(iter == range.second)
		return sim_mob::Red;
	else
	{
		return (*iter).second.currColor;
	}
}
/*checks current phase for the current color of the crossing(if the crossing found),
 * other cases and phases, just return red.
 */
TrafficColor Signal_SCATS::getPedestrianLight (Crossing const & crossing) const
{
	const sim_mob::Phase & phase = getCurrPhase();
	const sim_mob::Phase::crossings_map_const_iterator it = phase.getCrossingMaps().find((const_cast<Crossing *>(&crossing)));
	if(it != phase.getCrossingMaps().end())
	{
		return (*it).second.currColor;
	}
	return sim_mob::Red;
}


void Signal_SCATS::addSignalSite(centimeter_t /* xpos */, centimeter_t /* ypos */,
		std::string const & /* typeCode */, double /* bearing */) {
	// Not implemented yet.
}

/* Set Split plan  for the signal*/
//might not be very necessary(not in use)
void Signal_SCATS::setSplitPlan(sim_mob::SplitPlan plan)
{
	plan_ = plan;
}
void Signal_SCATS::initializePhases() {

	 /* you have each phase percentage from the choice set,
		so you may set the phase percntage and phase offset of each phase,
		then initialize phases(calculate its phase length, green time ...)*/
	std::vector<double> choice = plan_.CurrSplitPlan();
	if(choice.size() != getNOF_Phases())
		throw std::runtime_error("Mismatch on number of phases");
	int i = 0 ; double percentage_sum =0;
	//setting percentage and phaseoffset for each phase
//	std::cout << "Analyzing phase .for signal "<< this->getId() << std::endl;
//	std::cout << "nof phases = " << getPhases().size() << std::endl;
//	getchar();
	for(int ph_it = 0; ph_it < getPhases().size(); ph_it++, i++)
	{
		//this ugly line of code is due to the fact that multi index renders constant versions of its elements
//		std::cout << "Analyzing phase " << (getPhases()[ph_it]).getName()  << std::endl;
		sim_mob::Phase & target_phase = const_cast<sim_mob::Phase &>(getPhases()[ph_it]);
//		std::cout << " ....  done with casting phase " << target_phase.getName() << std::endl;
		if( i > 0) percentage_sum += choice[i - 1]; // i > 0 : the first phase has phase offset equal to zero,
		(target_phase).setPercentage(choice[i]);
		(target_phase).setPhaseOffset(percentage_sum * plan_.getCycleLength() / 100);
//		std::cout << "Signal " << this->getId() << "  Set phaseoffset for " << (target_phase).getPhaseName() << " to " << (target_phase).getPhaseOffset() << std::endl;
	}
	//Now Initialize the phases(later you  may put this back to the above phase iteration loop
	for(int ph_it = 0; ph_it < getPhases().size(); ph_it++, i++)
		const_cast<sim_mob::Phase &>((getPhases()[ph_it])).initialize();
	//.......................
}

/*Signal Initialization */
//might not be very necessary(not in use)
void Signal_SCATS::initialize() {
	createStringRepresentation("");

	//initialize phases......
//	plan_.initialize();
	NOF_Phases = getNOF_Phases();//remove this error prone line later
	initializePhases();
	Phase_Density.resize(getNOF_Phases(), 0);

}

/* Get Split plan  for the signal*/
//might not be very necessary(not in use)
sim_mob::SplitPlan & Signal_SCATS::getPlan()
{
	return plan_;
}

}//namespace

#endif
