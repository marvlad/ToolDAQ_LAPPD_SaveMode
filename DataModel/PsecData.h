#ifndef PSECDATA_H
#define PSECDATA_H

#include "zmq.hpp"
#include <SerialisableObject.h>
#include <iostream>
#include <vector>
#include <map>

using namespace std;

class PsecData{

 friend class boost::serialization::access;

 public:

    PsecData();

    vector<unsigned short> ReceiveData;
    vector<unsigned short> AccInfoFrame;
    vector<int> BoardIndex;
    vector<unsigned short> RawWaveform;
    map<int, vector<unsigned short>> TransferMap;
    vector<unsigned int> errorcodes;

    vector<int> LAPPDtoBoard1;
    vector<int> LAPPDtoBoard2;
    unsigned int ACC_ID;

    //map<int,int> FFCounter;
    int counter;
    int DataSaved;
    string time;
    string Timestamp;

    unsigned int VersionNumber;

    //Save 0
    ////Save Receive Data + rest

    //Save 1
    map<int,map<int, vector<unsigned short>>> ParseData;
    map<int,vector<unsigned short>> ParseMeta;

    //Save for all
    int FailedReadCounter;

    int readRetval;

    bool Print();

 private:

 template <class Archive> void serialize(Archive& ar, const unsigned int version){

  ar & VersionNumber;
  ar & ACC_ID;
  ar & BoardIndex;
  ar & RawWaveform;
  ar & AccInfoFrame;
  ar & FailedReadCounter;
  
 }

 
};

#endif
