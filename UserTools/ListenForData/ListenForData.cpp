#include "ListenForData.h"

ListenForData::ListenForData():Tool(){}


bool ListenForData::Initialise(std::string configfile, DataModel &data){

	if(configfile!="")  m_variables.Initialise(configfile);
	//m_variables.Print();

	m_data= &data;
	m_log= m_data->Log;

	if(!m_variables.Get("verbose",m_verbose)) m_verbose=1;
    	if(!m_variables.Get("PrintLinesMax",PrintLinesMax)) PrintLinesMax=50000;

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

	//Get Timestamp
	unsigned long long timeSinceEpoch = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	m_data->psec.Timestamp = to_string(timeSinceEpoch-UTCCONV); 

	vector<unsigned int> tmpERR = m_data->acc->returnErrors();
	m_data->psec.errorcodes.insert(std::end(m_data->psec.errorcodes), std::begin(tmpERR), std::end(tmpERR));
	m_data->acc->clearErrors();
	tmpERR.clear();
	if(m_verbose>1){SaveErrorLog();}

	return true;
}


bool ListenForData::Finalise(){
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

bool ListenForData::SaveErrorLog()
{
	int numLines = 0;
	std::string line;
	std::ifstream file("./LocalLog/Errorlog_ListenForData.txt");    
	while(getline(file, line)){numLines++;}
	file.close();

	if(numLines>PrintLinesMax){return false;}
	if(m_data->psec.errorcodes.size()==0){return false;}
	if(m_data->psec.errorcodes.size()==1)
	{
		if(m_data->psec.errorcodes.at(0)==0x00000000)
		{
			m_data->psec.errorcodes.clear();
			return false;
		}
	}

	//Create Debug file
	std::fstream outfile("./LocalLog/Errorlog_ListenForData.txt", std::ios_base::out | std::ios_base::app);

	//Print a timestamp
	outfile << "Time: " << m_data->psec.Timestamp << endl;
	for(int k1=0; k1<m_data->psec.errorcodes.size(); k1++)
	{
		if(m_data->psec.errorcodes.at(k1)!=0x00000000)
		{
			outfile << "0x" << std::hex << m_data->psec.errorcodes.at(k1) << std::dec <<  " ";
		}
	}
	outfile << endl;
	outfile.close();
	
	m_data->psec.errorcodes.clear();

	return true;
}
