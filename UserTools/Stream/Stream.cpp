#include "Stream.h"

Stream::Stream():Tool(){}


bool Stream::Initialise(std::string configfile, DataModel &data){

  if(configfile!="")  m_variables.Initialise(configfile);
  //m_variables.Print();

  m_data= &data;
  m_log= m_data->Log;

  if(!m_variables.Get("verbose",m_verbose)) m_verbose=1;
  if(!m_variables.Get("Port",m_port)) m_port=8888;

  sock=new zmq::socket_t(*(m_data->context), ZMQ_DEALER);
  std::stringstream tmp;
  tmp<<"tcp://127.0.0.1:"<<m_port;
  sock->bind(tmp.str().c_str());

  return true;
}


bool Stream::Execute(){
  PsecData *pdata;
  pdata = new PsecData;
  std::map<int, PsecData> StreamMap;

  for(std::map<int, vector<unsigned short>>::iterator it=m_data->psec.ReceiveData.begin(); it!=m_data->psec.ReceiveData.end(); ++it)
  {
  	StreamMap[it->first] = *pdata;
  	StreamMap[it->first].BoardIndex = it->first;
	  StreamMap[it->first].RawWaveform = it->second;
    StreamMap[it->first].AccInfoFrame = m_data->psec.AccInfoFrame;
    StreamMap[it->first].FailedReadCounter = m_data->psec.FailedReadCounter;
    StreamMap[it->first].Send(sock);
    StreamMap[it->first].Print();
  }
  
  return true;
}


bool Stream::Finalise(){
  delete sock;
  sock=0; 

  return true;
}
