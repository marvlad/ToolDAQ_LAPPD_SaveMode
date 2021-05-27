#include "ACDC.h"
#include <bitset>
#include <sstream>
#include <fstream>
#include <chrono> 
#include <iomanip>
#include <numeric>
#include <ctime>

using namespace std;

ACDC::ACDC(){}

ACDC::~ACDC(){}

int ACDC::getBoardIndex()
{
	return boardIndex;
}

void ACDC::setBoardIndex(int bi)
{
	boardIndex = bi;
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
			infobytes.push_back(*bit);
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









