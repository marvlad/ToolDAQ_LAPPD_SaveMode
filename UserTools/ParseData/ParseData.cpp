#include "ParseData.h"

ParseData::ParseData():Tool(){}


bool ParseData::Initialise(std::string configfile, DataModel &data){

    if(configfile!="")  m_variables.Initialise(configfile);
    //m_variables.Print();

    m_data= &data;
    m_log= m_data->Log;

    if(!m_variables.Get("verbose",m_verbose)) m_verbose=1;

    first=boost::posix_time::microsec_clock::local_time();

    return true;
}


bool ParseData::Execute(){
    bool retfinish=false;

    if(m_data->psec.readRetval!=0)
    {
        return true;
    }

    //Savemode check
    if(m_data->conf.Savemode==0)
    {
        retfinish = true;
    }else if(m_data->conf.Savemode==1)
    {
        int retval, type;
        if(m_data->psec.ReceiveData.size()%16==0)
        {
            type = 16; 
        }else if(m_data->psec.ReceiveData.size()%7795==0)
        {
            type = 7795;
        }
        int n_bo = m_data->psec.BoardIndex.size();

        for(int i=0; i<n_bo; i++)
        {
            for(int cj=i*type; cj<(i+1)*type; cj++)
            {
                m_data->psec.TransferMap[m_data->psec.BoardIndex.at(i)].push_back(m_data->psec.ReceiveData.at(cj));
            }
        }

        for(std::map<int, vector<unsigned short>>::iterator it=m_data->psec.TransferMap.begin(); it!=m_data->psec.TransferMap.end(); ++it)
        {
            //fill ParsedStream with vectors from data
            retval = getParsedData(it->second);
            if(retval == -3)
            {
                m_data->conf.Savemode = 3;
                return true;
            }else if(retval == 0)
            {
                m_data->psec.ParseData[it->first]=data;
                retval = getParsedMeta(it->second,it->first);
                if(retval!=0)
                {
                    std::cout << "Meta parsing went wrong! " << retval << std::endl;
                }
                m_data->psec.ParseMeta[it->first]=meta;	
            }else if(retval!=0 && retval!=-3)
            {
                std::cout << "Parsing went wrong! " << retval << std::endl;
            }
            channel_count = 0;
        }
        retfinish = true;
    }else if(m_data->conf.Savemode==2)
    {
        //Basic return raw 
        retfinish = true;
    }else if(m_data->conf.Savemode==3)
    {
        int retval, type;
        if(m_data->psec.ReceiveData.size()%16==0)
        {
            type = 16; 
        }else if(m_data->psec.ReceiveData.size()%7795==0)
        {
            type = 7795;
        }
        int n_bo = m_data->psec.BoardIndex.size();

        for(int i=0; i<n_bo; i++)
        {
            for(int cj=i*type; cj<(i+1)*type; cj++)
            {
                m_data->psec.TransferMap[m_data->psec.BoardIndex.at(i)].push_back(m_data->psec.ReceiveData.at(cj));
            }
        }

        //Basic return raw
        retfinish = true;
    }else if(m_data->conf.Savemode==4)
    {
        int retval, type;
        if(m_data->psec.ReceiveData.size()%16==0)
        {
            type = 16; 
        }else if(m_data->psec.ReceiveData.size()%7795==0)
        {
            type = 7795;
        }
        int n_bo = m_data->psec.BoardIndex.size();

        for(int i=0; i<n_bo; i++)
        {
            for(int cj=i*type; cj<(i+1)*type; cj++)
            {
                m_data->psec.TransferMap[m_data->psec.BoardIndex.at(i)].push_back(m_data->psec.ReceiveData.at(cj));
            }
        }

        for(std::map<int, vector<unsigned short>>::iterator it=m_data->psec.TransferMap.begin(); it!=m_data->psec.TransferMap.end(); ++it)
        {
            //fill ParsedStream with vectors from data
            retval = getParsedData(it->second);
            if(retval == -3)
            {
                m_data->conf.Savemode = 3;
                return true;
            }else if(retval == 0)
            {
                m_data->psec.ParseData[it->first]=data;
                retval = getParsedMeta(it->second,it->first);
                if(retval!=0)
                {
                    std::cout << "Meta parsing went wrong! " << retval << std::endl;
                }
                m_data->psec.ParseMeta[it->first]=meta;	
            }else if(retval!=0 && retval!=-3)
            {
                std::cout << "Parsing went wrong! " << retval << std::endl;
            }
            channel_count = 0;
        }
        retfinish = true;
    }else
    {
        std::cout << "Unknown save mode set!" << std::endl;
    }

    return retfinish;
}


bool ParseData::Finalise()
{
	return true;
}


int ParseData::getParsedMeta(std::vector<unsigned short> buffer, int classindex)
{
	//Catch empty buffers
	if(buffer.size() == 0)
	{
		std::cout << "You tried to parse ACDC data without pulling/setting an ACDC buffer" << std::endl;
		return -1;
	}

 	//Prepare the Metadata vector 
	meta.clear();

	//Helpers
	int chip_count = 0;

	//Indicator words for the start/end of the metadata
	const unsigned short startword = 0xBA11; 
	unsigned short endword = 0xFACE; 
    unsigned short endoffile = 0x4321;

	//Empty metadata map for each Psec chip <PSEC #, vector with information>
	map<int, vector<unsigned short>> PsecInfo;

	//Empty trigger metadata map for each Psec chip <PSEC #, vector with trigger>
	map<int, vector<unsigned short>> PsecTriggerInfo;
	unsigned short CombinedTriggerRateCount;

	//Empty vector with positions of aboves startword
	vector<int> start_indices=
    {
        1539, 3091, 4643, 6195, 7747
    }; 

	//Find the startwords and write them to the vector
	vector<unsigned short>::iterator bit;
	//Fill the psec info map
	for(int i: start_indices)
	{
		//Write the first word after the startword
		bit = buffer.begin() + (i+1);

		//As long as the endword isn't reached copy metadata words into a vector and add to map
		vector<unsigned short> InfoWord;
		while(*bit != endword && *bit != endoffile && InfoWord.size() < 14)
		{
			InfoWord.push_back(*bit);
			++bit;
		}
		PsecInfo.insert(pair<int, vector<unsigned short>>(chip_count, InfoWord));
		chip_count++;
	}

	//Fill the psec trigger info map
	for(int chip=0; chip<NUM_PSEC; chip++)
	{
		for(int ch=0; ch<NUM_CH/NUM_PSEC; ch++)
		{
			//Find the trigger data at begin + last_metadata_start + 13_info_words + 1_end_word + 1 
			bit = buffer.begin() + start_indices[4] + 13 + 1 + 1 + ch + (chip*(NUM_CH/NUM_PSEC));
			PsecTriggerInfo[chip].push_back(*bit);
		}
	}

	//Fill the combined trigger
	CombinedTriggerRateCount = buffer[7792];

	//----------------------------------------------------------
	//Start the metadata parsing 
	meta.push_back(classindex);
	for(int CHIP=0; CHIP<NUM_PSEC; CHIP++)
	{
		meta.push_back((0xDCB0 | CHIP));
		for(int INFOWORD=0; INFOWORD<13; INFOWORD++)
		{
			meta.push_back(PsecInfo[CHIP][INFOWORD]);		
		}
		for(int TRIGGERWORD=0; TRIGGERWORD<6; TRIGGERWORD++)
		{
			meta.push_back(PsecTriggerInfo[CHIP][TRIGGERWORD]);
		}
	}

	meta.push_back(CombinedTriggerRateCount);
	// dummy timestamp
        // ----------------------------------------------------------------------------------------------------
        int extra_timestamp = 1;
        if (extra_timestamp == 1) {
                unsigned int decimalNumber = static_cast<unsigned int>(std::time(nullptr));
                std::stringstream ss;
                ss << std::hex << std::setw(8) << std::setfill('0') << decimalNumber; // Ensure 8 characters in the output
                std::string hexString = ss.str();
                // Split the hex string into sets of 4 characters and convert each to unsigned short
                for (size_t i = 0; i < hexString.length(); i += 4) {
                    std::string sub = hexString.substr(i, 4);
                    unsigned short value = static_cast<unsigned short>(std::stoul(sub, nullptr, 16));
                    meta.push_back(value);
                }
        }
        // ----------------------------------------------------------------------------------------------------
	meta.push_back(0xeeee);
	return 0;
}


int ParseData::getParsedData(std::vector<unsigned short> buffer)
{
	//Catch empty buffers
	if(buffer.size() == 0)
	{
		std::cout << "You tried to parse ACDC data without pulling/setting an ACDC buffer" << std::endl;
		return -1;
	}
	if(buffer.size() == 16)
	{
		data.clear();
		return -3;	
	}

	//Prepare the Metadata vector 
	data.clear();
    channel_count=0;

	//Indicator words for the start/end of the metadata
	const unsigned short startword = 0xF005; 
	unsigned short endword = 0xBA11;
	unsigned short endoffile = 0x4321;

	//Empty vector with positions of aboves startword
	vector<int> start_indices=
    {
        2, 1554, 3106, 4658, 6210
    }; 

	//Find the startwords and write them to the vector
	vector<unsigned short>::iterator bit;
	//Fill data map
	for(int i: start_indices)
	{
		//Write the first word after the startword
		bit = buffer.begin() + (i+1);

		//As long as the endword isn't reached copy metadata words into a vector and add to map
		vector<unsigned short> InfoWord;
		while(*bit != endword && *bit != endoffile)
		{
			InfoWord.push_back((unsigned short)*bit);
			if(InfoWord.size()==NUM_SAMP)
			{
				data.insert(pair<int, vector<unsigned short>>(channel_count, InfoWord));
				InfoWord.clear();
				channel_count++;
			}
			++bit;
		}	
	}

	return 0;
}
