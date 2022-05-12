#include <Config.h>

Config::Config()
{
   SetDefaults(); 
}

bool Config::SetDefaults(){
  //ReceiveFlag
  receiveFlag=0;
  
  ResetSwitchACC=0;
  ResetSwitchACDC=0;
  Savemode=0;

  SMA=0;

  //trigger
  triggermode=1;

  //triggersettings

  ACC_Sign=0;

  ACDC_Sign=0;

  SELF_Sign=0;
  SELF_Enable_Coincidence=0;
  SELF_Coincidence_Number=0;
  SELF_threshold=1600;

  //ACDC boards
  ACDC_mask=0xFF;

  //PSEC chips for self trigger
  PSEC_Chip_Mask_0=1;
  PSEC_Chip_Mask_1=0;
  PSEC_Chip_Mask_2=0;
  PSEC_Chip_Mask_3=0;
  PSEC_Chip_Mask_4=0;
  PSEC_Channel_Mask_0=0x20;
  PSEC_Channel_Mask_1=0x00;
  PSEC_Channel_Mask_2=0x00;
  PSEC_Channel_Mask_3=0x00;
  PSEC_Channel_Mask_4=0x00;

  //Validation time
  Validation_Start=325000;
  Validation_Window=20000;

  //Calibration mode
  Calibration_Mode=0;

  //Raw mode
  Raw_Mode=true;

  //Pedestal set value channel
  Pedestal_channel=2000;
  Pedestal_channel_mask=0x1F;
	
  PPSRatio = 0x0001;
  PPSBeamMultiplexer = 1;

  return true;
}

bool Config::Print(){
  std::cout << "------------------General settings------------------" << std::endl;
  printf("Receive flag: %i\n", receiveFlag);
  printf("ACDC boardmask: 0x%02x\n",ACDC_mask);
  printf("Calibration Mode: %i\n",Calibration_Mode);
  printf("Raw_Mode: %i\n",(int)Raw_Mode);
  std::cout << "------------------Trigger settings------------------" << std::endl;
  printf("Triggermode: %i\n",triggermode);
  printf("ACC trigger Sign: %i\n", ACC_Sign);
  printf("ACDC trigger Sign: %i\n", ACDC_Sign);
  printf("Selftrigger Sign: %i\n", SELF_Sign);
  printf("Coincidence Mode: %i\n", SELF_Enable_Coincidence);
  printf("Required Coincidence Channels: %d\n", SELF_Coincidence_Number);
  printf("Selftrigger threshold: %d\n", SELF_threshold);
  printf("Validation trigger start: %f us\n", Validation_Start);
  printf("Validation trigger window: %f us\n", Validation_Window);
  std::cout << "------------------PSEC settings------------------" << std::endl;
  printf("PSEC chipmask (chip 0 to 4) : %i|%i|%i|%i|%i\n",PSEC_Chip_Mask_0,PSEC_Chip_Mask_1,PSEC_Chip_Mask_2,PSEC_Chip_Mask_3,PSEC_Chip_Mask_4);
  printf("PSEC channelmask (for chip 0 to 4) : 0x%02x|0x%02x|0x%02x|0x%02x|0x%02x\n",PSEC_Channel_Mask_0,PSEC_Channel_Mask_1,PSEC_Channel_Mask_2,PSEC_Channel_Mask_3,PSEC_Channel_Mask_4);
  printf("PSEC pedestal value: %d\n", Pedestal_channel);
  printf("PSEC chipmask for pedestal: 0x%02x\n", Pedestal_channel_mask);
  std::cout << "-------------------------------------------------" << std::endl;

	return true;
}
