#include "ListenForData.h"

ListenForData::ListenForData():Tool(){}


bool ListenForData::Initialise(std::string configfile, DataModel &data){

	if(configfile!="")  m_variables.Initialise(configfile);
	//m_variables.Print();

	m_data= &data;
	m_log= m_data->Log;

	if(!m_variables.Get("verbose",m_verbose)) m_verbose=1;

	timestamp = getTime();
	
	m_data->psec.FFCounter[0] = 0;
	m_data->psec.FFCounter[1] = 0;
	m_data->psec.FFCounter[2] = 0;
	m_data->psec.FFCounter[3] = 0;
	m_data->psec.FFCounter[4] = 0;
	m_data->psec.FFCounter[5] = 0;
	m_data->psec.FFCounter[6] = 0;
	m_data->psec.FFCounter[7] = 0;
	
	return true;
}


bool ListenForData::Execute(){
	int saveswitch;
	m_variables.Get("Save", saveswitch);

	if(m_data->conf.triggermode==1)
	{
		m_data->acc->softwareTrigger();
	}

	if (counter>=100000)
	{
		counter=0;
		timestamp = getTime();
	}
	
	if(saveswitch==0)
	{
		timestamp = "NoSave";	
	}


	m_data->psec.readRetval = m_data->acc->listenForAcdcData(m_data->conf.triggermode, m_data->conf.Raw_Mode, timestamp);
	if(m_data->psec.readRetval != 0)
	{
		m_data->psec.FailedReadCounter = m_data->psec.FailedReadCounter + 1;
		m_data->psec.ReceiveData.clear();
	}else if(m_data->psec.readRetval == 0)
	{
		counter++;
	}

	map<int, vector<unsigned short>> rawmap;
	rawmap = m_data->acc->returnRaw();
  	for(std::map<int, vector<unsigned short>>::iterator it=rawmap.begin(); it!=rawmap.end(); ++it)
  	{		
		if(it->second.at(0) != 0x1234)
		{
			m_data->psec.FFCounter[it->first] += 1;
		}
	}
	
	return true;
}


bool ListenForData::Finalise(){
	if(m_data->psec.FFCounter.size()>0){
		for(std::map<int, int>::iterator it=m_data->psec.FFCounter.begin(); it!=m_data->psec.FFCounter.end(); ++it)
  		{
			std::cout << "Board " << it->first << " has " << it->second << " ff buffers" << std::endl; 
		}
	}else{
		std::cout << "Trouble" << std::endl;
	}
	
	delete m_data->acc;
	return true;
}

string ListenForData::getTime()
{
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y%d%m_%H%M%S");
    return ss.str();
}
