#ifndef ParseData_H
#define ParseData_H

#include <string>
#include <iostream>
#include <map>
#include <vector>

#include "Tool.h"


#define NUM_CH 30
#define NUM_SAMP 256
#define NUM_PSEC 5

using namespace std;
/**
 * \class ParseData
 *
 * This is a balnk template for a Tool used by the script to generate a new custom tool. Please fill out the descripton and author information.
*
* $Author: B.Richards $
* $Date: 2019/05/28 10:44:00 $
* Contact: b.richards@qmul.ac.uk
*/
class ParseData: public Tool {


 public:

  ParseData(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resorces. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Executre function used to perform Tool perpose. 
  bool Finalise(); ///< Finalise funciton used to clean up resorces.

  int getParsedData(std::vector<unsigned short> buffer);
  int getParsedMeta(std::vector<unsigned short> buffer, int i);

 private:
  int channel_count=0;
  map<int, std::vector<unsigned short>> data;
  std::vector<unsigned short> meta;
  std::vector<unsigned short> pps;
  boost::posix_time::ptime first;

};


#endif
