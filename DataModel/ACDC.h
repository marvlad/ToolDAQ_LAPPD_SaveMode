#ifndef _ACDC_H_INCLUDEDreadDataFromFile
#define _ACDC_H_INCLUDED

#include <iostream>
#include <cstdlib>
#include <string>
#include <vector>
#include <map>
#include "Metadata.h" //load metadata class

using namespace std;

#define NUM_CH 30 //maximum number of channels for one ACDC board
#define NUM_PSEC 5 //maximum number of psec chips on an ACDC board
#define NUM_SAMP 256 //maximum number of samples of one waveform
#define NUM_CH_PER_CHIP 6 //maximum number of channels per psec chips

class ACDC
{
public:
	ACDC(); //constructor
	~ACDC(); //deconstructor

	//----------local return functions
	int getBoardIndex(); //get the current board index from the acdc
	int getNumCh() {int a = NUM_CH; return a;} //returns the number of total channels per acdc
	int getNumPsec() {int a = NUM_PSEC; return a;} //returns the number of psec chips on an acdc
	int getNumSamp() {int a = NUM_SAMP; return a;} //returns the number of samples for on event
	map<int, vector<double>> returnData(){return data;} //returns the entire data map | index: channel < samplevector
	map<string, unsigned short> returnMeta(){return map_meta;} //returns the entire meta map | index: metakey < value 

	//----------local set functions
	void setBoardIndex(int bi); // set the board index for the current acdc
	void setPeds(map<int, vector<double>> p, int bi){peds[bi] = p;} //sets pedestal map
	void setConv(map<int, vector<double>>& c){conv = c;} //sets adc-conversion map
	void setData(map<int, vector<double>>& d){data = d;} //sets data map

	//----------parse function for data stream 
	int parseDataFromBuffer(vector<unsigned short> acdc_buffer); //parses only the psec data component of the ACDC buffer

	//----------write data to file
	void writeRawBufferToFile(vector<unsigned short> lastAcdcBuffer); //write raw buffer to file for debugging 
	void writeRawDataToFile(vector<unsigned short> buffer, ofstream& d); //write raw buffer to file to save as regular data 
	void writeErrorLog(string errorMsg); //write errorlog with timestamps

	//----------read data from file
	void readPedsFromFile(ifstream& ifs, int bi); //read the pedestal values from file 
	void readConvsFromFile(ifstream& ifs); //read the conversion values from file

private:
	//----------all neccessary classes
	Metadata meta; //calls the metadata class for file write

	//----------all neccessary global variables
	int boardIndex; //var: represents the boardindex for the current board
	unsigned int trigMask; //var: triggermask
	vector<unsigned short> lastAcdcBuffer; //most recently received ACDC buffer
	map<int, vector<double>> data; //entire data map | index: channel < samplevector
	map<int, map<int, vector<double>>> peds; //entire ped map | index: board < channel < samplevector
	map<int, vector<double>> conv; //conversion factor from adc counts to mv from calibration file. 
	map<string, unsigned short> map_meta; //entire meta map | index: metakey < value
};

#endif
