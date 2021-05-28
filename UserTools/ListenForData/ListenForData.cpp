#include "ListenForData.h"

ListenForData::ListenForData():Tool(){}


bool ListenForData::Initialise(std::string configfile, DataModel &data){

	if(configfile!="")  m_variables.Initialise(configfile);
	//m_variables.Print();

	m_data= &data;
	m_log= m_data->Log;

	if(!m_variables.Get("verbose",m_verbose)) m_verbose=1;

	timestamp = getTime();

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

	//m_data->psec.ReceiveData = m_data->acc->returnRaw();

	return true;
}


bool ListenForData::Finalise(){
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
