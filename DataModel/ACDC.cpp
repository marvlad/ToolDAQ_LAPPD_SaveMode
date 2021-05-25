#include "ACDC.h"
#include <bitset>
#include <sstream>
#include <fstream>
#include <chrono> 
#include <iomanip>
#include <numeric>
#include <ctime>

using namespace std;

ACDC::ACDC()
{
	trigMask = 0xFFFFFF;
}



ACDC::~ACDC()
{
	cout << "Calling acdc destructor" << endl;
}

int ACDC::getBoardIndex()
{
	return boardIndex;
}

void ACDC::setBoardIndex(int bi)
{
	boardIndex = bi;
}


//utility for debugging
void ACDC::writeRawBufferToFile(vector<unsigned short> lastAcdcBuffer)
{
    string fnnn = "raw-acdc-buffer.txt";
    cout << "Printing ACDC buffer to file : " << fnnn << endl;
    ofstream cb(fnnn);
    for(unsigned short k: lastAcdcBuffer)
    {
        cb << hex << k << endl;
    }
    cb.close();
}

//looks at the last ACDC buffer and organizes
//all of the data into a data map. The boolean
//argument toggles whether you want to subtract
//pedestals and convert ADC-counts to mV live
//or keep the data in units of raw ADC counts. 
//retval: 
//2: other error
//1: corrupt buffer 
//0: all good
int ACDC::parseDataFromBuffer(vector<unsigned short> acdc_buffer)
{
	vector<unsigned short> acdcBuffer = acdc_buffer;

	//make sure an acdc buffer has been
	//filled. if not, there is nothing to be done.
	if(acdcBuffer.size() == 0)
	{
		string err_msg = "You tried to parse ACDC data without pulling/setting an ACDC buffer";
		writeErrorLog(err_msg);
		return 2;
	}

	//clear the data map prior.
	data.clear();

	//if the buffer is 0 length (i.e. bad usb comms)
	//return doing nothing
	if(acdcBuffer.size() == 0)
	{
		return true;
	}
	int dist;
	int channel_count=1;

	//byte that indicates the metadata of
	//each psec chip is about to follow. 
	const unsigned short startword = 0xF005; 
	unsigned short endword = 0xBA11;
	unsigned short endoffile = 0x4321;

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
        	if(start_indices.size()!=0 && abs(dist-start_indices[start_indices.size()])<(6*256+15))
        	{
            	continue;        
        	}
        	start_indices.push_back(dist);
        }
	}
	
	if(start_indices.size()>NUM_PSEC)
	{
		for(int k=0; k<(int)start_indices.size()-1; k++)
		{
			if(start_indices[k+1]-start_indices[k]>6*256+14)
			{
				//nothing
			}else
			{
				start_indices.erase(start_indices.begin()+(k+1));
				k--;
			}
		}
	}

    bool corruptBuffer = false;
	if(start_indices.size() != NUM_PSEC)
	{
        string err_msg = "In parsing ACDC buffer, found ";
        err_msg += to_string(start_indices.size());
        err_msg += " psec data flag bytes.";
        writeErrorLog(err_msg);
        string fnnn = "acdc-corrupt-psec-buffer.txt";
        cout << "Printing to file : " << fnnn << endl;
        ofstream cb(fnnn);
        for(unsigned short k: acdcBuffer)
        {
            cb << hex << k << endl;
        }
        corruptBuffer = true;
	}

	for(int i: start_indices)
	{
		//re-use buffer iterator from above
		//to set starting point. 
		bit = acdcBuffer.begin() + i + 1; //the 1 is to start one element after the startword
		//while we are not at endword, 
		//append elements to ac_info
		vector<double> infobytes;
		while(*bit != endword && *bit != endoffile)
		{
			infobytes.push_back((double)*bit);
			if(infobytes.size()==NUM_SAMP)
			{
				data[channel_count] = infobytes;
				infobytes.clear();
				channel_count++;
			}
			++bit;
		}	
	}

	if(data.size()!=NUM_CH)
	{
		cout << "error 1" << endl;
		corruptBuffer = true; 
	}

	for(int i=0; i<NUM_CH; i++)
	{
		if(data[i+1].size()!=NUM_SAMP)
		{
			cout << "error 2" << endl;
		}
	}

	bool corruptMetaBuffer;
	corruptMetaBuffer = meta.parseBuffer(acdcBuffer);

	map_meta = meta.getMetadata();

	if(corruptMetaBuffer)
	{
		return 3;
	}	
	if(corruptBuffer)
	{
		return 2;
	}

	return 0;
}

//writes data from the presently stored event
// to file assuming file has header already
void ACDC::writeRawDataToFile(vector<unsigned short> buffer, ofstream& d)
{
	for(unsigned short k: buffer)
	{
		d << hex <<  k << " ";
	}
	d << endl;
	d.close();
	return;
}

//reads pedestals to file in a new
//file format relative to the old software. 
//<channel> <sample 1> <sample 2> ...
//<channel> ...
//samples in ADC counts. 
void ACDC::readPedsFromFile(ifstream& ifs, int bi)
{
	char delim = ' '; //in between ADC counts
	map<int, vector<double>> tempPeds;//temporary holder for the new pedestal map


	//temporary variables for line parsing
	string lineFromFile; //full line
	string adcCountStr; //string representing adc counts of ped
	double avg; //int for the current channel key
	int ch=0;

	//loop over each line of file
	for(int i=0; i<NUM_SAMP; i++)
	{
		getline(ifs, lineFromFile);
		stringstream line(lineFromFile); //stream of characters delimited

		//loop over each sample index
		for(int j=0; j<NUM_CH; j++)
		{
			getline(line, adcCountStr, delim);
			avg = stod(adcCountStr); //channel key for a while
			tempPeds[j].push_back(avg);
		}
	}

	//call public member of this class to set the pedestal map
	setPeds(tempPeds, bi);
}


//reads LUT conversions to file in a new
//file format relative to the old software. 
//<channel> <sample 1> <sample 2> ...
//<channel> ...
//samples in ADC counts. 
void ACDC::readConvsFromFile(ifstream& ifs)
{
	char delim = ' '; //in between ADC counts
	map<int, vector<double>> tempConvs;//temporary holder for the new conversion map

	//temporary variables for line parsing
	string lineFromFile; //full line
	string adcCountStr; //string representing adc counts of conv
	int ch; //int for the current channel key
	vector<double> tempWav; //conv wav temporary 
	bool isChannel; //is this character the channel key

	//loop over each line of file
	while(getline(ifs, lineFromFile))
	{
		stringstream line(lineFromFile); //stream of characters delimited
		isChannel = true; //first character is the channel key
		tempWav.clear(); //fresh vector
		//loop over each sample index
		while(getline(line, adcCountStr, delim))
		{
			if(isChannel)
			{
				ch = stoi(adcCountStr); //channel key for a while
				isChannel = false;
				continue; //go to next delimited word (start of adcCounts)
			}
			tempWav.push_back(stod(adcCountStr)); //conversions in adcCounts
		}

		//now set this vector to the appropriate conv map element
		tempConvs.insert(pair<int, vector<double>>(ch, tempWav));
	}

	//call public member of this class to set the conversion map
	setConv(tempConvs);
	return;
}

void ACDC::writeErrorLog(string errorMsg)
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









