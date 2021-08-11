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

  bool Send(zmq::socket_t* sock);
  bool Receive(zmq::socket_t* sock);

  map<int, vector<unsigned short>> ReceiveData;
  map<int,int> FFCounter;
  int Savemode;
  int counter=0;
  string time;

  int BoardIndex;
  unsigned int VersionNumber = 0x0001;

  vector<unsigned short> RawWaveform;

  //Save 0
  ////Save Receive Data + rest

  //Save 1
  map<int,map<int, vector<unsigned short>>> Parse1;
  map<int,vector<unsigned short>> Meta1;

  //Save 2
  map<int, vector<unsigned short>> Parse2;
  vector<unsigned short> Meta2;

  //Save for all
  vector<unsigned short> AccInfoFrame;
  vector<unsigned short> PPS;
  int FailedReadCounter=0;

  int readRetval;

  bool Print();

 private:

 template <class Archive> void serialize(Archive& ar, const unsigned int version){

  ar & VersionNumber;
  ar & BoardIndex;
  ar & RawWaveform;
  ar & AccInfoFrame;
  ar & FailedReadCounter;
  
 }

 
};

#endif
