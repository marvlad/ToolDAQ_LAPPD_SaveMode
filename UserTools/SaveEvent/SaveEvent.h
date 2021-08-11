#ifndef SaveEvent_H
#define SaveEvent_H

#include <string>
#include <iostream>
#include <map>
#include <vector>
#include <chrono>

#include "Tool.h"


/**
 * \class CreateAnnieEvent
 *
 * This is a balnk template for a Tool used by the script to generate a new custom tool. Please fill out the descripton and author information.
*
* $Author: B.Richards $
* $Date: 2019/05/28 10:44:00 $
* Contact: b.richards@qmul.ac.uk
*/
class SaveEvent: public Tool {


 public:

  SaveEvent(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resorces. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Executre function used to perform Tool perpose. 
  bool Finalise(); ///< Finalise funciton used to clean up resorces.


 private:

    string path;
    string WaveformLabel;
    string AccLabel;
    string MetaLabel;
    string PPSLabel;

    void SaveRaw();
    void SaveASCII(string time);

    string getTime()
    {
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&in_time_t), "%Y%d%m_%H%M%S");
        return ss.str();
    }



};


#endif
