#include "ListenForData.h"

ListenForData::ListenForData():Tool(){}


bool ListenForData::Initialise(std::string configfile, DataModel &data){

	if(configfile!="")  m_variables.Initialise(configfile);
	//m_variables.Print();

	m_data= &data;
	m_log= m_data->Log;

	if(!m_variables.Get("verbose",m_verbose)) m_verbose=1;
	/*Old debugging tool 
        m_data->psec.FFCounter.insert(pair<int,int>(0, 0));
        m_data->psec.FFCounter.insert(pair<int,int>(1, 0));
        m_data->psec.FFCounter.insert(pair<int,int>(2, 0));
        m_data->psec.FFCounter.insert(pair<int,int>(3, 0));
        m_data->psec.FFCounter.insert(pair<int,int>(4, 0));
        m_data->psec.FFCounter.insert(pair<int,int>(5, 0));
        m_data->psec.FFCounter.insert(pair<int,int>(6, 0));
        m_data->psec.FFCounter.insert(pair<int,int>(7, 0));
	*/
	return true;
}


bool ListenForData::Execute()
{
	if(m_data->conf.triggermode==1)
	{
		m_data->acc->softwareTrigger();
	}

	m_data->psec.readRetval = m_data->acc->listenForAcdcData(m_data->conf.triggermode);
	if(m_data->psec.readRetval!=0)
	{
        if(m_data->psec.readRetval!=404)
        {
		    m_data->psec.FailedReadCounter = m_data->psec.FailedReadCounter + 1;
        }
        m_data->psec.ReceiveData.clear();
		m_data->psec.AccInfoFrame.clear();
        m_data->psec.BoardIndex.clear();
        m_data->acc->clearData();
	}else
	{
		m_data->psec.AccInfoFrame = m_data->acc->returnACCIF();
		m_data->psec.ReceiveData = m_data->acc->returnRaw();
		m_data->psec.BoardIndex = m_data->acc->returnBoardIndices();
		m_data->acc->clearData();
	}

    /*
        map<int, vector<unsigned short>> rawmap;
        rawmap = m_data->psec.ReceiveData;
        for(std::map<int, vector<unsigned short>>::iterator it=rawmap.begin(); it!=rawmap.end(); ++it)
        {		
            if(it->second.at(0) != 0x1234)
            {
                m_data->psec.FFCounter[it->first] += 1;
            }
        }
	*/
	return true;
}


bool ListenForData::Finalise(){
	/*
        if(m_data->psec.FFCounter.size()>0){
            for(std::map<int, int>::iterator it=m_data->psec.FFCounter.begin(); it!=m_data->psec.FFCounter.end(); ++it)
            {
                std::cout << "Board " << it->first << " has " << it->second << " ff buffers" << std::endl; 
            }
        }else{
            std::cout << "Trouble" << std::endl;
        }
	*/
	delete m_data->acc;
    m_data->acc=0;
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
