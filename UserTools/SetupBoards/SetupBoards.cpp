#include "SetupBoards.h"

SetupBoards::SetupBoards():Tool(){}


bool SetupBoards::Initialise(std::string configfile, DataModel &data){

	if(configfile!="")  m_variables.Initialise(configfile);
	//m_variables.Print();

	system("mkdir -p Results");
	
	std::cout << "----------------------------------------------------------------------" << std::endl;
	std::cout << "                                 Hello                                " << std::endl;
	std::cout << "                         This is version v2.30                        " << std::endl;
	std::cout << " The latest changes were:                                             " << std::endl;
    std::cout << " - Large Update fixing all sorts of things                            " << std::endl;
    std::cout << " - Added a new Savemode that alllows for dual saving                  " << std::endl;
	std::cout << "----------------------------------------------------------------------" << std::endl;

	m_data= &data;
	m_log= m_data->Log;

	if(!m_variables.Get("verbose",m_verbose)) m_verbose=1;

	m_data->conf.receiveFlag=0;
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

	return true;
}


bool SetupBoards::Execute(){
	
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

        int timeout;
        m_variables.Get("Timeout",timeout);
        m_data->acc->setTimeoutInMs(timeout);

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
			return false;
		}else
		{
			std::cout << "Initialization successfull!" << std::endl;
		}

		m_data->conf.receiveFlag = 2;

		m_data->acc->emptyUsbLine();
		m_data->acc->dumpData(0xFF);
	}else
    {
        std::cout << "Setup failed miserably!" << std::endl;
        return false;
    }

	if(m_data->psec.readRetval!=0)
	{
		m_data->acc->dumpData(0xFF);
		m_data->acc->emptyUsbLine();
	}

	return true;
}


bool SetupBoards::Finalise()
{
	return true;
}


bool SetupBoards::LoadSettings()
{
    m_variables.Get("Triggermode",m_data->conf.triggermode);	

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
