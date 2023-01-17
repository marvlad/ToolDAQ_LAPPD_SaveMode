#include "SetupBoards.h"

SetupBoards::SetupBoards():Tool(){}


bool SetupBoards::Initialise(std::string configfile, DataModel &data){

	if(configfile!="")  m_variables.Initialise(configfile);
	//m_variables.Print();

	system("mkdir -p Results");
    	system("mkdir -p LocalLogs");

    	if(!m_variables.Get("versionnumber",versionnumber)) versionnumber="Nan";
	
	std::cout << "----------------------------------------------------------------------" << std::endl;
	std::cout << "                                 Hello                                " << std::endl;
	std::cout << "                         This is version ";std::cout<<versionnumber<<"                    " << std::endl;
	std::cout << " The latest changes were:                                             " << std::endl;
    	std::cout << " - See changelog.txt                                                  " << std::endl;
	std::cout << "----------------------------------------------------------------------" << std::endl;

	m_data= &data;
	m_log= m_data->Log;

	if(!m_variables.Get("verbose",m_verbose)) m_verbose=1;

	m_data->conf.receiveFlag=1;
	m_data->acc= new ACC();

	bool ret = LoadSettings();
	m_variables.Get("Save",m_data->conf.Savemode);

	if(m_data->conf.Savemode==2 || m_data->conf.Savemode==4)
	{
		// Make the ANNIEEvent Store if it doesn't exist
		int recoeventexists = m_data->Stores.count("LAPPDStore");
		if(recoeventexists==0)
		{
			m_data->Stores["LAPPDStore"] = new BoostStore(false,2);
		}
	}

	if(!m_variables.Get("TimeoutMax",TimeoutMax)) TimeoutMax=300;
	if(!m_variables.Get("PrintLinesMax",PrintLinesMax)) PrintLinesMax=50000;

	Timeoutcounter = 0;

	return true;
}


bool SetupBoards::Execute(){

	if(Timeoutcounter>=TimeoutMax)
	{
		Timeoutcounter = 0; //Reset the timeout counter 

		//IF THERE ARE PROBLEMS 
		//COMMENT FROM HERE -----------
		m_data->conf.receiveFlag = 1; //Re-init the Setup part uf the tool

		//Print debug frame as overwrite
		int numLines = 0;
		std::string line;
		std::ifstream file("./LocalLogs/ACCIF.txt");
		while(getline(file, line)){numLines++;}
		file.close();

		if(numLines<PrintLinesMax && m_verbose>1){PrintDebugFrames();}
		PrintSettings();
		//TO HERE -------------
	}

	bool setupret = false;
	if(m_data->conf.receiveFlag==1)
	{
		if(m_data->conf.ResetSwitchACC==1)
		{
			m_data->acc->resetACC();
		}
		if(m_data->conf.ResetSwitchACDC==1)
		{
			m_data->acc->resetACDC();
		}
        	setupret = Setup();
	}else if(m_data->conf.receiveFlag==2)
	{
		//
	}else
	{
		std::cout << "Setup failed miserably!" << std::endl;
		return false;
	}

    	if(m_data->psec.readRetval!=0)
	{
		if(m_data->psec.readRetval==404)
		{
            		Timeoutcounter++;
            		vector<unsigned short> tempV = m_data->acc->getACCInfoFrame();
			for(int i=0; i<MAX_NUM_BOARDS; i++)
			{
				if(tempV.at(16+i)!=PSECFRAME && tempV.at(16+i)!=PPSFRAME)
				{
					if(m_verbose>1){std::cout<<"Data buffers were: "<<i<<" -> "<<tempV.at(16+i)<<std::endl;}
					m_data->acc->dumpData(0xFF);
					m_data->acc->emptyUsbLine();
                		}
            		}
            		tempV.clear();
			if(m_verbose>2)
			{
				m_data->conf.Print();
			}
		}else
		{
			m_data->psec.errorcodes.push_back(0xAA02EE01);
			PrintReturnFrame();
			m_data->acc->dumpData(0xFF);
			m_data->acc->emptyUsbLine();
		}

        	if(m_verbose>1){SaveErrorLog();}
	}else
	{
		Timeoutcounter = 0;
	}

	return true;
}


bool SetupBoards::Finalise()
{
	return true;
}


bool SetupBoards::Setup()
{
	bool ret=false;

	int timeout;
	m_variables.Get("Timeout",timeout);
	m_data->acc->setTimeoutInMs(timeout);

	//Map boards to LAPPDs
	m_data->acc->setLAPPD1(m_data->conf.LAPPDtoBoard1);
	m_data->psec.LAPPDtoBoard1 = m_data->conf.LAPPDtoBoard1;
	m_data->acc->setLAPPD2(m_data->conf.LAPPDtoBoard2);
	m_data->psec.LAPPDtoBoard2 = m_data->conf.LAPPDtoBoard2;
	m_data->psec.ACC_ID = m_data->conf.ACC_ID;

	//trigger settings
	////polarity
	m_data->acc->setSign(m_data->conf.ACC_Sign, 2);
	m_data->acc->setSign(m_data->conf.ACDC_Sign, 3);
	m_data->acc->setSign(m_data->conf.SELF_Sign, 4);

	////self trigger options
	m_data->acc->setEnableCoin(m_data->conf.SELF_Enable_Coincidence);

	unsigned int coinNum;
	stringstream ss;
	ss << std::hex << m_data->conf.SELF_Coincidence_Number;
	coinNum = std::stoul(ss.str(),nullptr,16);
	m_data->acc->setNumChCoin(coinNum);

	unsigned int threshold;
	stringstream ss2;
	ss2 << std::hex << m_data->conf.SELF_threshold;
	threshold = std::stoul(ss2.str(),nullptr,16);
	m_data->acc->setThreshold(threshold);

	//psec masks combine
	std::vector<int> PsecChipMask = {m_data->conf.PSEC_Chip_Mask_0,m_data->conf.PSEC_Chip_Mask_1,m_data->conf.PSEC_Chip_Mask_2,m_data->conf.PSEC_Chip_Mask_3,m_data->conf.PSEC_Chip_Mask_4};
	std::vector<unsigned int> VecPsecChannelMask = {m_data->conf.PSEC_Channel_Mask_0,m_data->conf.PSEC_Channel_Mask_1,m_data->conf.PSEC_Channel_Mask_2,m_data->conf.PSEC_Channel_Mask_3,m_data->conf.PSEC_Channel_Mask_4};
	m_data->acc->setPsecChipMask(PsecChipMask);
	m_data->acc->setPsecChannelMask(VecPsecChannelMask);

	//validation window
	unsigned int validationStart;
	stringstream ss31;
	ss31 << std::hex << (int)m_data->conf.Validation_Start/25;
	validationStart = std::stoul(ss31.str(),nullptr,16);
	m_data->acc->setValidationStart(validationStart);		

	unsigned int validationWindow;
	stringstream ss32;
	ss32 << std::hex << (int)m_data->conf.Validation_Window/25;
	validationWindow = std::stoul(ss32.str(),nullptr,16);
	m_data->acc->setValidationWindow(validationWindow);

	//pedestal set
	////set value
	unsigned int pedestal;
	stringstream ss4;
	ss4 << std::hex << m_data->conf.Pedestal_channel;
	pedestal = std::stoul(ss4.str(),nullptr,16);
	////set mask
	m_data->acc->setPedestals(m_data->conf.ACDC_mask,m_data->conf.Pedestal_channel_mask,pedestal);


	//pps settings
	unsigned int ppsratio;
	stringstream ss5;
	ss5 << std::hex << m_data->conf.PPSRatio;
	ppsratio = std::stoul(ss5.str(),nullptr,16);
	m_data->acc->setPPSRatio(ppsratio);

	m_data->acc->setPPSBeamMultiplexer(m_data->conf.PPSBeamMultiplexer);

	if(m_data->conf.SMA==0)
	{
		m_data->acc->setSMA_OFF();
	}else if(m_data->conf.SMA==1)
	{
		m_data->acc->setSMA_ON();
	}
        
	int retval;
	retval = m_data->acc->initializeForDataReadout(m_data->conf.triggermode, m_data->conf.ACDC_mask, m_data->conf.Calibration_Mode);
	if(retval != 0)
	{
		std::cout << "Initialization failed!" << std::endl;
		ret = false;
	}else
	{
		std::cout << "Initialization successfull!" << std::endl;
		ret = true;
	}

	m_data->conf.receiveFlag = 2;

	vector<unsigned int> tmpERR = m_data->acc->returnErrors();
	m_data->psec.errorcodes.insert(std::end(m_data->psec.errorcodes), std::begin(tmpERR), std::end(tmpERR));
	tmpERR.clear();

	m_data->acc->emptyUsbLine();
	m_data->acc->dumpData(0xFF);

	return ret;
}

bool SetupBoards::LoadSettings()
{
    m_variables.Get("Triggermode",m_data->conf.triggermode);	

    std::vector<int> vtmp;
    int itmp;
    m_variables.Get("LAPPD1_1",itmp);
    vtmp.push_back(itmp);
    m_variables.Get("LAPPD1_2",itmp);
    vtmp.push_back(itmp);   
    m_data->conf.LAPPDtoBoard1 = vtmp;
    vtmp.clear();
    m_variables.Get("LAPPD2_1",itmp);
    vtmp.push_back(itmp);
    m_variables.Get("LAPPD2_2",itmp);
    vtmp.push_back(itmp);   
    m_data->conf.LAPPDtoBoard2 = vtmp;
    vtmp.clear();

    m_variables.Get("SMA",m_data->conf.SMA);
    m_variables.Get("ACC_Sign",m_data->conf.ACC_Sign);
    m_variables.Get("ACDC_Sign",m_data->conf.ACDC_Sign);
    m_variables.Get("SELF_Sign",m_data->conf.SELF_Sign);

    m_variables.Get("SELF_Enable_Coincidence",m_data->conf.SELF_Enable_Coincidence);
    m_variables.Get("SELF_Coincidence_Number",m_data->conf.SELF_Coincidence_Number);
    m_variables.Get("SELF_threshold",m_data->conf.SELF_threshold);

    m_variables.Get("PSEC_Chip_Mask_0",m_data->conf.PSEC_Chip_Mask_0);
    m_variables.Get("PSEC_Chip_Mask_1",m_data->conf.PSEC_Chip_Mask_1);
    m_variables.Get("PSEC_Chip_Mask_2",m_data->conf.PSEC_Chip_Mask_2);
    m_variables.Get("PSEC_Chip_Mask_3",m_data->conf.PSEC_Chip_Mask_3);
    m_variables.Get("PSEC_Chip_Mask_4",m_data->conf.PSEC_Chip_Mask_4);

    string tempPsecChannelMask;
    m_variables.Get("PSEC_Channel_Mask_0",tempPsecChannelMask);
    m_data->conf.PSEC_Channel_Mask_0 = std::stoul(tempPsecChannelMask,nullptr,16);
    m_variables.Get("PSEC_Channel_Mask_1",tempPsecChannelMask);
    m_data->conf.PSEC_Channel_Mask_1 = std::stoul(tempPsecChannelMask,nullptr,16);
    m_variables.Get("PSEC_Channel_Mask_2",tempPsecChannelMask);
    m_data->conf.PSEC_Channel_Mask_2 = std::stoul(tempPsecChannelMask,nullptr,16);
    m_variables.Get("PSEC_Channel_Mask_3",tempPsecChannelMask);
    m_data->conf.PSEC_Channel_Mask_3 = std::stoul(tempPsecChannelMask,nullptr,16);
    m_variables.Get("PSEC_Channel_Mask_4",tempPsecChannelMask);
    m_data->conf.PSEC_Channel_Mask_4 = std::stoul(tempPsecChannelMask,nullptr,16);	

    m_variables.Get("Validation_Start",m_data->conf.Validation_Start);
    m_variables.Get("Validation_Window",m_data->conf.Validation_Window);

    m_variables.Get("Pedestal_channel",m_data->conf.Pedestal_channel);
    m_variables.Get("Pedestal_channel_mask",tempPsecChannelMask);
    m_data->conf.Pedestal_channel_mask = std::stoul(tempPsecChannelMask,nullptr,16);

    m_variables.Get("ACDC_mask",tempPsecChannelMask);
    m_data->conf.ACDC_mask = std::stoul(tempPsecChannelMask,nullptr,16);		

    m_variables.Get("Calibration_Mode",m_data->conf.Calibration_Mode);
    m_variables.Get("Raw_Mode",m_data->conf.Raw_Mode);
    
    string tempPPSRatio;
    m_variables.Get("PPS_Ratio",tempPPSRatio);
    m_data->conf.PPSRatio = std::stoul(tempPPSRatio,nullptr,16);	
    m_variables.Get("PPS_Mux",m_data->conf.PPSBeamMultiplexer);

    m_variables.Get("resetACC",m_data->conf.ResetSwitchACC);
    m_variables.Get("resetACDC",m_data->conf.ResetSwitchACDC);

    m_data->conf.receiveFlag=1;

    return true;
}

void SetupBoards::PrintReturnFrame()
{
	//Create Debug file
	std::fstream outfile("./LocalLogs/ReturnFrame.txt", std::ios_base::out | std::ios_base::app);

	//Print a timestamp
	outfile << "Time: " << m_data->psec.Timestamp << endl;
	outfile << "Unexpected return value = " << m_data->psec.readRetval << endl;

	//Grab first ACC info frame and get all buffer sizes that are present
	vector<unsigned short> PrintFrame = m_data->acc->getACCInfoFrame();
	for(int j=0; j<8; j++)
	{
		if(PrintFrame.at(14) & (1 << j))
		{
			outfile << "W" << j << ": " << PrintFrame.at(16+j) << " | ";
		}
	}
	outfile << endl;

	//Clear temp vector plus sleep a bit
	PrintFrame.clear();
	usleep(100000);

	//Grab second ACC info frame and get all buffer sizes that are present
	PrintFrame = m_data->acc->getACCInfoFrame();
	for(int j2=0; j2<8; j2++)
	{
		if(PrintFrame.at(14) & (1 << j2))
		{
			outfile << "W" << j2 << ": " << PrintFrame.at(16+j2) << " | ";
		}
	}
	outfile << endl;

	//Close and clear
	outfile.close();
	PrintFrame.clear();
}


void SetupBoards::PrintSettings()
{
    std::fstream outfile("./LocalLogs/FoundSettings.txt", std::ios_base::out | std::ios_base::trunc);

    outfile << "------------------LAPPD to Board mappig-------------" << std::endl;
    outfile << "Will come soon" << std::endl;
    outfile << "LAPPD 1 is mapped to boards " << m_data->conf.LAPPDtoBoard1[0] << " and " << m_data->conf.LAPPDtoBoard1[1] << std::endl;
    outfile << "LAPPD 2 is mapped to boards " << m_data->conf.LAPPDtoBoard2[0] << " and " << m_data->conf.LAPPDtoBoard2[1] << std::endl;
    outfile << "------------------General settings------------------" << std::endl;
    outfile << "Receive flag: " <<  m_data->conf.receiveFlag << std::endl;
    outfile << "ACDC boardmask: " <<  m_data->conf.ACDC_mask << std::endl;
    outfile << "Calibration Mode: " <<  m_data->conf.Calibration_Mode << std::endl;
    outfile << "Raw_Mode: " <<  m_data->conf.Raw_Mode << std::endl;
    outfile << "------------------Trigger settings------------------" << std::endl;
    outfile << "Triggermode: " << m_data->conf.triggermode << std::endl;
    outfile << "ACC trigger Sign: " << m_data->conf.ACC_Sign << std::endl;
    outfile << "ACDC trigger Sign: " << m_data->conf.ACDC_Sign << std::endl;
    outfile << "Selftrigger Sign: " << m_data->conf.SELF_Sign << std::endl;
    outfile << "Coincidence Mode: " <<  m_data->conf.SELF_Enable_Coincidence << std::endl;
    outfile << "Required Coincidence Channels: " <<  m_data->conf.SELF_Coincidence_Number << std::endl;
    outfile << "Selftrigger threshold: " << m_data->conf.SELF_threshold << std::endl;
    outfile << "Validation trigger start: " << m_data->conf.Validation_Start << std::endl;
    outfile << "Validation trigger window: " << m_data->conf.Validation_Window << std::endl;
    outfile << "------------------PSEC settings------------------" << std::endl;
    outfile << "PSEC chipmask (chip 0 to 4) : " << m_data->conf.PSEC_Chip_Mask_0 << "|" << m_data->conf.PSEC_Chip_Mask_1 << "|" << m_data->conf.PSEC_Chip_Mask_2 << "|" << m_data->conf.PSEC_Chip_Mask_3 << "|" << m_data->conf.PSEC_Chip_Mask_4 << std::endl;
    outfile << "PSEC channelmask (for chip 0 to 4) : " << m_data->conf.PSEC_Channel_Mask_0 << "|" << m_data->conf.PSEC_Channel_Mask_1 << "|" << m_data->conf.PSEC_Channel_Mask_2 << "|" << m_data->conf.PSEC_Channel_Mask_3 << "|" << m_data->conf.PSEC_Channel_Mask_4 << std::endl;
    outfile << "PSEC pedestal value: " << m_data->conf.Pedestal_channel << std::endl;
    outfile << "PSEC chipmask for pedestal: " << m_data->conf.Pedestal_channel_mask << std::endl;
    outfile << "------------------PSEC settings------------------" << std::endl;	
    outfile << "PPS divider ratio: " << m_data->conf.PPSRatio << std::endl;
    outfile << "PPS multiplexer: " << m_data->conf.PPSBeamMultiplexer << std::endl;
    outfile << "-------------------------------------------------" << std::endl;

    outfile.close();
}


bool SetupBoards::SaveErrorLog()
{
	int numLines = 0;
	std::string line;
	std::ifstream file("./LocalLogs/Errorlog_Setup.txt");    
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
	std::fstream outfile("./LocalLogs/Errorlog_Setup.txt", std::ios_base::out | std::ios_base::app);

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
