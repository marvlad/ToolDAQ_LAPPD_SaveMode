#include "Metadata.h"
#include <algorithm>
#include <sstream>
#include <bitset>
#include <iostream>
#include <cmath>
#include <chrono> 
#include <iomanip>
#include <numeric>
#include <ctime>

using namespace std;

Metadata::Metadata()
{
	initializeMetadataKeys();
}

Metadata::Metadata(vector<unsigned short> acdcBuffer)
{
	initializeMetadataKeys();
	parseBuffer(acdcBuffer);
}

Metadata::~Metadata()
{
}


//----------Massive parsing functions are below. 
//two metadatas that are known externally need to be set by ACDC class.
void Metadata::setBoardAndEvent(unsigned short board, int event)
{
    //there is a problem in that the map has been
    //defined assuming all metadata are unsigned shorts. 
    //but this only goes to 2^16 events (65536). If this happens, 
    //restart the count at zero
    event = event % 65536;
	checkAndInsert("Event", (unsigned short)event);
	checkAndInsert("Board", board);
}

int Metadata::getEventNumber()
{
    return (int)metadata["Event"];
}

//takes the full acdcBuffer as input. 
//splits it into a map[key][vector<unsigned short>]
//which is then parsed and organized in this class. 
//Returns:
//false if a corrupt buffer happened
//true if all good. 
bool Metadata::parseBuffer(vector<unsigned short> acdcBuffer)
{
	//if the buffer is 0 length (i.e. bad usb comms)
	//return doing nothing
	if(acdcBuffer.size() == 0) return true;
	int dist;

	//byte that indicates the metadata of
	//each psec chip is about to follow. 
	const unsigned short startword = 0xBA11; 

	//to hold data temporarily. local to this function. 
	//this is used to match to my old convention
	//from the old software (which took a while just)
	//to type out properly. 
	//ac_info[chip][index in buffer]
	map<int, vector<unsigned short>> ac_info;

	//this is the header to the ACDC buffer that
	//the ACC appends at the beginning, found in
	//packetUSB.vhd. 
	vector<unsigned short> cc_header_info;

	
	//indices of elements in the acdcBuffer
	//that correspond to the byte ba11
	vector<int> start_indices; 
	vector<unsigned short>::iterator bit;

	//loop through the data and find locations of startwords. 
    //this can be made more efficient if you are having efficiency problems.
	for(bit = acdcBuffer.begin(); bit != acdcBuffer.end(); ++bit)
	{
		//the iterator is at an element with startword value. 
		//push the index (integer, from std::distance) to a vector. 
        if(*bit == startword)
        {
        	dist= std::distance(acdcBuffer.begin(), bit);
        	if(start_indices.size()!=0 && abs(dist-start_indices[start_indices.size()])<6*256)
        	{
          		continue;  
        	}
        	start_indices.push_back(dist);
        }
	}

	//I have found experimentally that sometimes
    //the ACC sends an ACDC buffer that has 8001 elements
    //(correct) but has BAD data, i.e. with extra ADC samples
    //and missing PSEC metadata (startwords). This event
    //needs to be thrown away really, but here all I can 
    //do is return and say that the metadata is nothing. 
    bool corruptBuffer = false;
	if(start_indices.size() != NUM_PSEC)
	{
        string err_msg = "In parsing ACDC buffer, found ";
        err_msg += to_string(start_indices.size());
        err_msg += " matadata flag bytes.";
        writeErrorLog(err_msg);
		cout << "Metadata for this event will likely be jarbled. Please throw this out!" << endl;
        string fnnn = "acdc-corrupt-buffer.txt";
        cout << "Printing to file : " << fnnn << endl;
        ofstream cb(fnnn);
        for(unsigned short k: acdcBuffer)
        {
            cb << hex << k << endl;
        }
        corruptBuffer = true;
	}


	//loop through each startword index and store metadata. 
	int chip_count = 0;
	unsigned short endword = 0xFACE; //end of info buffer. 
    unsigned short endoffile = 0x4321;
	for(int i: start_indices)
	{
		//re-use buffer iterator from above
		//to set starting point. 
		bit = acdcBuffer.begin() + i + 1; //the 1 is to start one element after the startword
		//while we are not at endword, 
		//append elements to ac_info
		vector<unsigned short> infobytes;
		while(*bit != endword && *bit != endoffile && infobytes.size() < 20)
		{
			infobytes.push_back(*bit);
			++bit;
		}
		ac_info.insert(pair<int, vector<unsigned short>>(chip_count, infobytes));
		chip_count++;
	}

    map<int, unsigned short> trigger_info;
    vector<unsigned short> buffer;
    for(int ch=0; ch<NUM_CH; ch++)
    {
        bit = acdcBuffer.begin() + ch + start_indices[4]+15;
        trigger_info[ch] = *bit;
    }
    //trigger_info.insert(pair<int, unsigned short>(ch, bit));
    unsigned short combined_trigger = acdcBuffer[7792];

    //Here is where bad access errors could occur. 
    //It is presently coded such that if there are 
    //more ba11 bytes than NUM_PSEC (sometimes a bug
    //that happens at firmware level), nothing will go wrong
    //but this function will return a corruptBuffer flag at the end. 
    //However, if there are less than NUM_SEC ba11 sets of words
    //in the ac_info vector, we will have an access error and need
    //to return now. 
    if(ac_info.size() < NUM_PSEC)
    {
    	corruptBuffer = true;
        return corruptBuffer;
    }

    //Filling all the metadata infos with ac_info[chip][infoNr -1]
    //General PSEC info 
    for(int i = 0; i < NUM_PSEC; i++)
    {
        checkAndInsert("feedback_count_"+to_string(i), ac_info[i][0]); //Info 1
        checkAndInsert("feedback_target_count_"+to_string(i), ac_info[i][1]); //Info 2
        checkAndInsert("Vbias_setting_"+to_string(i), ac_info[i][2]); //Info 3
        checkAndInsert("selftrigger_threshold_setting_"+to_string(i), ac_info[i][3]); //Info 4
        checkAndInsert("PROVDD_setting_"+to_string(i), ac_info[i][4]); // Info 5
        checkAndInsert("VCDL_count_lo_"+to_string(i), ac_info[i][10]); //Info 11, later bit(15-0)
        checkAndInsert("VCDL_count_hi_"+to_string(i), ac_info[i][11]); //Info 12, later bit(31-16)
        checkAndInsert("DLLVDD_setting_"+to_string(i), ac_info[i][12]); //Info 13
    }

    if(ac_info[0][5] != 0xEEEE)
    {
    	corruptBuffer = true;
        string err_msg = "PSEC frame data, trigger_info (0,0) at psec info 6 is not right";
        writeErrorLog(err_msg);
        return corruptBuffer;
    }
    if(ac_info[4][5] != 0xEEEE)
    {
    	corruptBuffer = true;
        string err_msg = "PSEC frame data, trigger_info (0,4) at psec info 6 is not right";
        writeErrorLog(err_msg);
        return corruptBuffer;
    }
    //Trigger PSEC settings
    checkAndInsert("trigger_mode", ac_info[1][5] & 0xF); //Info 6, PSEC1 bit(3-0)
    checkAndInsert("trigger_validation_window_start", (ac_info[1][5] & 0xFFF0) >> 4); //Info 6, PSEC1 bit(15-4)
    checkAndInsert("trigger_validation_window_length", ac_info[2][5] & 0xFFF); //info 6, PSEC2 bit(11-0)
    checkAndInsert("trigger_sma_invert", (ac_info[3][5] & 0x2) >> 1); // Info 6, PSEC3 bit(1)
    checkAndInsert("trigger_sma_detection_mode", ac_info[3][5] & 0x1); // Info 6, PSEC3 bit(0)
    checkAndInsert("trigger_acc_invert", (ac_info[3][5] & 0x8) >> 3); // Info 6, PSEC3 bit(3)
    checkAndInsert("trigger_acc_detection_mode", (ac_info[3][5] & 0x4) >> 2); // Info 6, PSEC3 bit(2)
    checkAndInsert("trigger_self_sign", (ac_info[3][5] & 0x20) >> 5); //Info 6, PSEC3 bit(5)
    checkAndInsert("trigger_self_detection_mode", (ac_info[3][5] & 0x1) >> 4); // Info 6, PSEC3 bit(4)
    checkAndInsert("trigger_self_coin", (ac_info[3][5] & 0x7C0) >> 6); // Info 6, PSEC3 bit(10-6)

    checkAndInsert("trigger_selfmask_0", ac_info[0][6]); //Info 7 PSEC0
    checkAndInsert("trigger_selfmask_1", ac_info[1][6]); //Info 7 PSEC1
    checkAndInsert("trigger_selfmask_2", ac_info[2][6]); //Info 7 PSEC2
    checkAndInsert("trigger_selfmask_3", ac_info[3][6]); //Info 7 PSEC3
    checkAndInsert("trigger_selfmask_4", ac_info[4][6]); //Info 7 PSEC4

    checkAndInsert("trigger_self_threshold_0", ac_info[0][7] & 0xFFF); //Info 8 PSEC0 bit(11-0)
    checkAndInsert("trigger_self_threshold_1", ac_info[1][7] & 0xFFF); //Info 8 PSEC1 bit(11-0)
    checkAndInsert("trigger_self_threshold_2", ac_info[2][7] & 0xFFF); //Info 8 PSEC2 bit(11-0)
    checkAndInsert("trigger_self_threshold_3", ac_info[3][7] & 0xFFF); //Info 8 PSEC3 bit(11-0)
    checkAndInsert("trigger_self_threshold_4", ac_info[4][7] & 0xFFF); //Info 8 PSEC4 bit(11-0)

    //Timestamp data
    checkAndInsert("timestamp_0", ac_info[0][8]); // Info 9 PSEC0 later bit(15-0)
    checkAndInsert("timestamp_1", ac_info[1][8]); // Info 9 PSEC1 later bit(31-16)
    checkAndInsert("timestamp_2", ac_info[2][8]); // Info 9 PSEC2 later bit(47-32)
    checkAndInsert("timestamp_3", ac_info[3][8]); // Info 9 PSEC3 later bit(63-48)

    checkAndInsert("clockcycle_bits", ac_info[0][8] & 0x7); // Info 9 PSEC0 bit(2-0)

    //Event count
    checkAndInsert("event_count_lo", ac_info[0][9]); //Info 10 PSEC0 later bit(15-0)
    checkAndInsert("event_count_hi", ac_info[1][9]); //Info 10 PSEC1 later bit(31-16)

    for(int ch=0; ch<NUM_CH; ch++)
    {
        checkAndInsert("self_trigger_rate_count_psec_ch"+to_string(ch), trigger_info[ch]);
    }
    checkAndInsert("combined_trigger_rate_count", combined_trigger);

	return corruptBuffer;
}


//just makes sure not to insert elements
//into metadata map if they already exist. 
void Metadata::checkAndInsert(string key, unsigned short val)
{
	//if the key exists, change the value
    if(metadata.count(key) > 0)
    {
        metadata[key] = val;
    }
    //if it is new, insert
    else
    {
        metadata.insert(pair<string, unsigned short>(key, val));
    }
    return;
}



//keeps the metadata strings in a consistent
//order. Initialized in constructor and is used
//to order the printing/output of metadata map. 
void Metadata::initializeMetadataKeys()
{
    metadata_keys.push_back("Event");
    metadata_keys.push_back("Board");
    //General PSEC info 
    for(int i = 0; i < NUM_PSEC; i++)
    {
        metadata_keys.push_back("feedback_count_"+to_string(i)); //Info 1
        metadata_keys.push_back("feedback_target_count_"+to_string(i)); //Info 2
        metadata_keys.push_back("Vbias_setting_"+to_string(i)); //Info 3
        metadata_keys.push_back("selftrigger_threshold_setting_"+to_string(i)); //Info 4
        metadata_keys.push_back("PROVDD_setting_"+to_string(i)); // Info 5
        metadata_keys.push_back("VCDL_count_lo_"+to_string(i)); //Info 11, later bit(15-0)
        metadata_keys.push_back("VCDL_count_hi_"+to_string(i)); //Info 12, later bit(31-16)
        metadata_keys.push_back("DLLVDD_setting_"+to_string(i)); //Info 13
    }

    //Trigger PSEC settings
    metadata_keys.push_back("trigger_mode"); //Info 6, PSEC1 bit(3-0)
    metadata_keys.push_back("trigger_validation_window_start"); //Info 6, PSEC1 bit(15-4)
    metadata_keys.push_back("trigger_validation_window_length"); //info 6, PSEC2 bit(11-0)
    metadata_keys.push_back("trigger_sma_invert"); // Info 6, PSEC3 bit(1)
    metadata_keys.push_back("trigger_sma_detection_mode"); // Info 6, PSEC3 bit(0)
    metadata_keys.push_back("trigger_acc_invert"); // Info 6, PSEC3 bit(3)
    metadata_keys.push_back("trigger_acc_detection_mode"); // Info 6, PSEC3 bit(2)
    metadata_keys.push_back("trigger_self_sign"); //Info 6, PSEC3 bit(5)
    metadata_keys.push_back("trigger_self_detection_mode"); // Info 6, PSEC3 bit(4)
    metadata_keys.push_back("trigger_self_coin"); // Info 6, PSEC3 bit(10-6)

    metadata_keys.push_back("trigger_selfmask_0"); //Info 7 PSEC0
    metadata_keys.push_back("trigger_selfmask_1"); //Info 7 PSEC1
    metadata_keys.push_back("trigger_selfmask_2"); //Info 7 PSEC2
    metadata_keys.push_back("trigger_selfmask_3"); //Info 7 PSEC3
    metadata_keys.push_back("trigger_selfmask_4"); //Info 7 PSEC4

    metadata_keys.push_back("trigger_self_threshold_0"); //Info 8 PSEC0 bit(11-0)
    metadata_keys.push_back("trigger_self_threshold_1"); //Info 8 PSEC1 bit(11-0)
    metadata_keys.push_back("trigger_self_threshold_2"); //Info 8 PSEC2 bit(11-0)
    metadata_keys.push_back("trigger_self_threshold_3"); //Info 8 PSEC3 bit(11-0)
    metadata_keys.push_back("trigger_self_threshold_4"); //Info 8 PSEC4 bit(11-0)

    //Timestamp data
    metadata_keys.push_back("timestamp_0"); // Info 9 PSEC0 later bit(15-0)
    metadata_keys.push_back("timestamp_1"); // Info 9 PSEC1 later bit(31-16)
    metadata_keys.push_back("timestamp_2"); // Info 9 PSEC2 later bit(47-32)
    metadata_keys.push_back("timestamp_3"); // Info 9 PSEC3 later bit(63-48)

    metadata_keys.push_back("clockcycle_bits"); // Info 9 PSEC0 bit(2-0)

    //Event count
    metadata_keys.push_back("event_count_lo"); //Info 10 PSEC0 later bit(15-0)
    metadata_keys.push_back("event_count_hi"); //Info 10 PSEC1 later bit(31-16)

    for(int ch=0; ch<NUM_CH; ch++)
    {
         metadata_keys.push_back("self_trigger_rate_count_psec_ch"+to_string(ch));
    }
    metadata_keys.push_back("combined_trigger_rate_count");
}

void Metadata::writeErrorLog(string errorMsg)
{
    string err = "errorlog.txt";
    cout << "------------------------------------------------------------" << endl;
    cout << errorMsg << endl;
    cout << "------------------------------------------------------------" << endl;
    ofstream os_err(err, ios_base::app);
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%m-%d-%Y %X");
    os_err << "------------------------------------------------------------" << endl;
    os_err << ss.str() << endl;
    os_err << errorMsg << endl;
    os_err << "------------------------------------------------------------" << endl;
    os_err.close();
}
