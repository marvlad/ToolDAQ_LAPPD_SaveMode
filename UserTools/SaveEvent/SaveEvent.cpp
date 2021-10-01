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
  m_variables.Get("WaveformLabel",WaveformLabel);
  m_variables.Get("AccLabel",AccLabel);
  m_variables.Get("MetaLabel",MetaLabel);
  m_variables.Get("PPSLabel",PPSLabel);

  m_data->psec.time = getTime();

  return true;
}


bool SaveEvent::Execute(){
	if(m_data->psec.Savemode==0)
	{
		//nothing
	}else if(m_data->psec.Savemode==3)
	{
		if(m_data->psec.counter>10000)
		{
			m_data->psec.counter=0;
			m_data->psec.time = getTime();
		}
		SaveRaw();
		m_data->psec.counter +=1;
		
	}else if(m_data->psec.Savemode==1)
	{
		if(m_data->psec.counter>1000)
		{
			m_data->psec.counter=0;
			m_data->psec.time = getTime();
		}
		SaveASCII(m_data->psec.time);
		m_data->psec.counter +=1;
			
	}else if(m_data->psec.Savemode==2)
	{
		//Prepare temporary vectors
		std::map<unsigned long, vector<Waveform<double>>> LAPPDWaveforms;
		Waveform<double> tmpWave;
		vector<Waveform<double>> VecTmpWave;

		if(m_data->psec.PPS.size() == 0)
		{	
			//Loop over data stream
			for(std::map<int, vector<unsigned short>>::iterator it=m_data->psec.Parse2.begin(); it!=m_data->psec.Parse2.end(); ++it)
			{
				for(unsigned short k: it->second)
				{
					tmpWave.PushSample((double)k);
				}
				VecTmpWave.push_back(tmpWave);
				LAPPDWaveforms.insert(LAPPDWaveforms.end(),std::pair<unsigned long, vector<Waveform<double>>>((unsigned long)it->first,VecTmpWave));
				tmpWave.ClearSamples();
				VecTmpWave.clear();
			}	
		}

		m_data->Stores["LAPPDStore"]->Set(WaveformLabel,LAPPDWaveforms);
		m_data->Stores["LAPPDStore"]->Set(AccLabel,m_data->psec.AccInfoFrame);
		m_data->Stores["LAPPDStore"]->Set(MetaLabel,m_data->psec.Meta2);
		m_data->Stores["LAPPDStore"]->Set(PPSLabel,m_data->psec.PPS);
		m_data->Stores["LAPPDStore"]->Save(path.c_str()); //std::cout << "SAVED" << std::endl;	
		m_data->Stores["LAPPDStore"]->Delete(); 
		LAPPDWaveforms.clear();
	}else
	{
		std::cout << "Nothing received!" << std::endl;	
	}
	
	//Cleanup	
	m_data->psec.Parse1.clear();
	m_data->psec.Parse2.clear();
	m_data->psec.Meta1.clear();
	m_data->psec.Meta2.clear();
	m_data->psec.AccInfoFrame.clear();
	m_data->psec.PPS.clear();
	m_data->psec.ReceiveData.clear();
	return true;
}


bool SaveEvent::Finalise(){
	if(m_data->psec.Savemode==2)
	{
		m_data->Stores["LAPPDStore"]->Close();
		delete m_data->Stores["LAPPDStore"];
		m_data->Stores["LAPPDStore"] = 0;
		usleep(1000000);
		std::string datapath = path;
		BoostStore *indata=new BoostStore(false,2); //this leaks but its jsut for testing
		indata->Initialise(datapath);
		std::cout <<"Print indata:"<<std::endl;
		indata->Print(false);
		long entries;
		indata->Header->Get("TotalEntries",entries);
		std::cout <<"entries: "<<entries<<std::endl;
	}


	return true;
}


void SaveEvent::SaveRaw(string time)
{
	//Direct raw save of data
	for(std::map<int, vector<unsigned short>>::iterator it=m_data->psec.ReceiveData.begin(); it!=m_data->psec.ReceiveData.end(); ++it)
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

	vector<int> boardsReadyForRead;
	for(std::map<int,map<int, vector<unsigned short>>>::iterator it=m_data->psec.Parse1.begin(); it!=m_data->psec.Parse1.end(); ++it)
	{
		boardsReadyForRead.push_back(it->first);
	} 

	map<int,map<int, vector<unsigned short>>> map_data = m_data->psec.Parse1;
	map<int, vector<unsigned short>> map_meta = m_data->psec.Meta1;

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
}
