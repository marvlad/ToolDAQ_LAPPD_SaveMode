#include "SaveEvent.h"

SaveEvent::SaveEvent():Tool(){}


bool SaveEvent::Initialise(std::string configfile, DataModel &data){

    if(configfile!="")  m_variables.Initialise(configfile);
    //m_variables.Print();

    m_data= &data;
    m_log= m_data->Log;

    if(!m_variables.Get("verbose",m_verbose)) m_verbose=1;

    m_variables.Get("path",path);
    path+= getTime();
    m_variables.Get("StoreLabel",StoreLabel);
    m_variables.Get("EventsPerFile",EvtsPerFile);
    
    m_data->psec.time = getTime();

    starttime = getTime();

    return true;
}


bool SaveEvent::Execute(){
    if(m_data->psec.readRetval!=0){return true;}

	if(m_data->conf.Savemode==0)
	{
		//nothing
	}else if(m_data->conf.Savemode==1)
	{
		if(m_data->psec.counter>EvtsPerFile)
		{
			m_data->psec.counter=0;
			m_data->psec.time = getTime();
		}
		SaveASCII(m_data->psec.time);
		m_data->psec.counter +=1;	
	}else if(m_data->conf.Savemode==2)
	{
        m_data->psec.RawWaveform = m_data->psec.ReceiveData;
		m_data->Stores["LAPPDStore"]->Set(StoreLabel,m_data->psec);
		m_data->Stores["LAPPDStore"]->Save(path.c_str());
		m_data->Stores["LAPPDStore"]->Delete(); 
	}else if(m_data->conf.Savemode==3)
	{
		if(m_data->psec.counter>EvtsPerFile)
		{
			m_data->psec.counter=0;
			m_data->psec.time = getTime();
		}
		SaveRaw(m_data->psec.time);
		m_data->psec.counter +=1;
		
	}else if(m_data->conf.Savemode==4)
	{
		if(m_data->psec.counter>EvtsPerFile)
		{
			m_data->psec.counter=0;
			m_data->psec.time = getTime();
		}
		SaveASCII(m_data->psec.time);
		m_data->psec.counter +=1;

        m_data->psec.RawWaveform = m_data->psec.ReceiveData;
		m_data->Stores["LAPPDStore"]->Set(StoreLabel,m_data->psec);
		m_data->Stores["LAPPDStore"]->Save(path.c_str());
		m_data->Stores["LAPPDStore"]->Delete(); 
	}else
	{
		std::cout << "Nothing received!" << std::endl;	
	}
	
	//Cleanup	
	m_data->psec.ParseData.clear();
	m_data->psec.ParseMeta.clear();
	m_data->psec.AccInfoFrame.clear();
	m_data->psec.BoardIndex.clear();
	m_data->psec.ReceiveData.clear();
    m_data->psec.RawWaveform.clear();
    m_data->psec.TransferMap.clear();

    m_data->psec.DataSaved++;
    int MaxEvents;
    m_variables.Get("MaxEvents",MaxEvents);
    if(m_data->psec.DataSaved==MaxEvents)
    {
        m_data->vars.Set("StopLoop",1);
    }

	return true;
}


bool SaveEvent::Finalise()
{
	if(m_data->conf.Savemode==2 || m_data->conf.Savemode==4)
	{
		m_data->Stores["LAPPDStore"]->Close();
		delete m_data->Stores["LAPPDStore"];
		m_data->Stores["LAPPDStore"] = 0;
		//usleep(1000000);
        /*
            std::string datapath = path;
            BoostStore *indata=new BoostStore(false,2); //this leaks but its jsut for testing
            indata->Initialise(datapath);
            std::cout <<"Print indata:"<<std::endl;
            indata->Print(false);
            long entries;
            indata->Header->Get("TotalEntries",entries);
            std::cout <<"entries: "<<entries<<std::endl;
        */
	}

    std::tm tm = {};
    std::stringstream ss(starttime);
    ss >> std::get_time(&tm, "%Y%d%m_%H%M%S");
    auto start = std::chrono::system_clock::from_time_t(std::mktime(&tm));

    auto end = chrono::system_clock::now();
    auto dt = chrono::duration_cast<chrono::milliseconds>(end - start).count();
    cout << "Runtime was" << to_string(dt) << " ms"  << endl;

	return true;
}


void SaveEvent::SaveRaw(string time)
{
	//Direct raw save of data
	for(std::map<int, vector<unsigned short>>::iterator it=m_data->psec.TransferMap.begin(); it!=m_data->psec.TransferMap.end(); ++it)
	{
		string rawfn = "./Results/Raw_b" + to_string(it->first) + "_" + time + ".txt";
		ofstream d(rawfn.c_str(), ios::app); 
		for(unsigned short k: it->second)
		{
			d << hex <<  k << " ";
		}
		d << endl;
		d.close();
	}
}

void SaveEvent::SaveASCII(string time)
{
	string rawfn = "./Results/Ascii" + time + ".txt";
	ofstream d(rawfn.c_str(), ios::app); 

	vector<int> boardsReadyForRead = m_data->psec.BoardIndex;

	map<int,map<int, vector<unsigned short>>> map_data = m_data->psec.ParseData;
	map<int, vector<unsigned short>> map_meta = m_data->psec.ParseMeta;

	string delim = " ";
	for(int enm=0; enm<NUM_SAMP; enm++)
	{
		d << dec << enm << delim;
		for(int bi: boardsReadyForRead)
		{
			if(map_data[bi].size()==0)
			{
				cout << "Mapdata is empty" << endl;
			}
			for(int ch=0; ch<NUM_CH; ch++)
			{
				if(enm==0)
				{
					//cout << "Writing board " << bi << " and ch " << ch << ": " << map_data[bi][ch+1][enm] << endl;
				}
				d << dec << (unsigned short)map_data[bi][ch][enm] << delim;
			}
			if(enm<(int)map_meta[bi].size())
			{
				d << hex << map_meta[bi][enm] << delim;

			}else
			{
				d << 0 << delim;
			}
		}
		d << endl;
	}
	d.close();

    boardsReadyForRead.clear();
    map_data.clear();
    map_meta.clear();
}
