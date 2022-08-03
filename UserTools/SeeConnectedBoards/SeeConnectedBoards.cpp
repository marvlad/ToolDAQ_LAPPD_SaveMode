#include "SeeConnectedBoards.h"

SeeConnectedBoards::SeeConnectedBoards():Tool(){}


bool SeeConnectedBoards::Initialise(std::string configfile, DataModel &data){

	if(configfile!="")  m_variables.Initialise(configfile);
	//m_variables.Print();

	m_data= &data;
	m_log= m_data->Log;

	if(!m_variables.Get("verbose",m_verbose)) m_verbose=1;

    
    m_data->acc = new ACC();

	return true;
}


bool SeeConnectedBoards::Execute()
{
    m_data->acc->versionCheck();
    
	return true;
}


bool SeeConnectedBoards::Finalise()
{
	delete m_data->acc;
    m_data->acc=0;
	return true;
}

