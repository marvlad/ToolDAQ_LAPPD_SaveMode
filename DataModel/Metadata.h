#ifndef _METADATA_H_INCLUDED
#define _METADATA_H_INCLUDED

#include <string>
#include <map>
#include <vector>
#include <fstream>

#define NUM_PSEC 5 //maximum number of psec chips on an ACDC board
#define NUM_CH 30 //maximum number of channels for one ACDC board
#define NUM_CH_PER_PSEC 6 //maximum number of channels per psec chips

using namespace std;

class Metadata
{
public:
	Metadata(); //empty constructor
	Metadata(vector<unsigned short> acdcBuffer); //if a buffer exists already to parse
	~Metadata(); //deconstructor

	//----------local return functions
	int getEventNumber(); //returns the event number
	map<string, unsigned short> getMetadata(){return metadata;} //returns the metadata map | metakey < value
	vector<string> getMetaKeys(){return metadata_keys;} //returns the metakeys seperatly

	//----------local set functions
	void setBoardAndEvent(unsigned short board, int event); //sets the eventnumber and boardindex as metadata

	//----------parse function for metadata stream
	void checkAndInsert(string key, unsigned short val); //inserts vals into metadata map.
	bool parseBuffer(vector<unsigned short> acdcBuffer); //returns success or fail 1/0 and parses the buffer
	
	//----------write functions
	void writeErrorLog(string errorMsg); //writes the errorlog with timestamps

private:
	//----------all neccessary global variables
	map<string, unsigned short> metadata; //var: metadata map | metakeys < value
	vector<string> metadata_keys; //var: metadata keys

	//----------general functions
	void initializeMetadataKeys(); //initilizes the metadata keys
	
};


#endif