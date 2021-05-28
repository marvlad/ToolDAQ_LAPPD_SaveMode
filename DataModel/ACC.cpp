#include "ACC.h" 
#include <cstdlib> 
#include <bitset> 
#include <sstream> 
#include <string> 
#include <thread> 
#include <algorithm> 
#include <thread> 
#include <fstream> 
#include <atomic> 
#include <signal.h> 
#include <unistd.h> 
#include <cstring>
#include <chrono> 
#include <iomanip>
#include <numeric>
#include <ctime>

using namespace std;

/*ID:3+4 sigint handling*/
std::atomic<bool> quitacc(false); 
void ACC::got_signal(int){quitacc.store(true);}

/*------------------------------------------------------------------------------------*/
/*--------------------------------Constructor/Deconstructor---------------------------*/

/*ID:5 Constructor*/
ACC::ACC()
{
	bool clearCheck;
	usb = new stdUSB();
	if(!usb->isOpen())
	{
		writeErrorLog("Usb was unable to connect to ACC");
		delete usb;
		exit(EXIT_FAILURE);
	}

	clearCheck = emptyUsbLine();
	if(clearCheck==false)
	{
		writeErrorLog("Failed to clear USB line on constructor!");
	}
}

/*ID:6 Destructor*/
ACC::~ACC()
{
	bool clearCheck;
	cout << "Calling acc destructor" << endl;
	clearAcdcs();
	clearCheck = emptyUsbLine();
	if(clearCheck==false)
	{
		writeErrorLog("Failed to clear USB line on destructor!");
	}
	delete usb;
}


/*------------------------------------------------------------------------------------*/
/*---------------------------Setup functions for ACC/ACDC-----------------------------*/

/*ID:9 Create ACDC class instances for each connected ACDC board*/
int ACC::createAcdcs()
{
	//To prepare clear the USB line just in case
	bool clearCheck = emptyUsbLine();
	if(clearCheck==false)
	{
		writeErrorLog("Failed to clear USB line before creating ACDCs!");
	}

	//Check for connected ACDC boards
	int retval = whichAcdcsConnected(); 
	if(retval==-1)
	{
		std::cout << "Trying to reset ACDC boards" << std::endl;
		unsigned int command = 0xFFFF0000;
		usb->sendData(command);
		usleep(1000000);
		int retval = whichAcdcsConnected();
		if(retval==-1)
		{
			std::cout << "After ACDC reset no changes, still no boards found" << std::endl;
		}
	}

	//if there are no ACDCs, return 0
	if(alignedAcdcIndices.size() == 0)
	{
		writeErrorLog("No aligned ACDC indices");
		return 0;
	}
	
	//Clear the ACDC class vector if one exists
	clearAcdcs();

	//Create ACDC objects with their board numbers
	//loaded into alignedAcdcIndices in the last function call. 
	for(int bi: alignedAcdcIndices)
	{
		ACDC* temp = new ACDC();
		temp->setBoardIndex(bi);
		acdcs.push_back(temp);
	}
	if(acdcs.size()==0)
	{
		writeErrorLog("No ACDCs created even though there were boards found!");
		return 0;
	}

	return 1;
}

/*ID:11 Queries the ACC for information about connected ACDC boards*/
int ACC::whichAcdcsConnected()
{
	int retval=0;
	unsigned int command;
	vector<int> connectedBoards;

	//New sequence to ask the ACC to reply with the number of boards connected 
	//Disables the PSEC4 frame data transfer for this sequence. Has to be set to HIGH later again
	enableTransfer(0); 
	usleep(100000);

	//Resets the RX buffer on all 8 ACDC boards
	command = 0x000200FF; 
	usbcheck=usb->sendData(command); if(usbcheck==false){writeErrorLog("Send Error");}
	//Sends a reset for detected ACDC boards to the ACC
	command = 0x00030000; 
	usbcheck=usb->sendData(command); if(usbcheck==false){writeErrorLog("Send Error");}
	//Request a 32 word ACDC ID frame containing all important infomations
	command = 0xFFD00000; 
	usbcheck=usb->sendData(command); if(usbcheck==false){writeErrorLog("Send Error");}

	usleep(100000);

	//Request and read the ACC info buffer and pass it the the corresponding vector
	command=0x00200000;
	usbcheck=usb->sendData(command); if(usbcheck==false){writeErrorLog("Send Error");}
	lastAccBuffer = usb->safeReadData(SAFE_BUFFERSIZE);

	//Check if buffer size is 32 words
	if(lastAccBuffer.size() != ACCFRAME) 
	{
		string err_msg = "Something wrong with ACC buffer, size: ";  
		err_msg += to_string(lastAccBuffer.size());
		writeErrorLog(err_msg);
		return 0;
	}


	unsigned short alignment_packet = lastAccBuffer.at(7);	
	for(int i = 0; i < MAX_NUM_BOARDS; i++)
	{	
		if(lastAccBuffer.at(16+i) == ACDCFRAME && (lastAccBuffer.at(14) & (1 << i)))
		{
			cout << "Board "<< i << " with 32 words after ACC buffer read, ";
			cout << "Board "<< i << " connected" << endl;
		}else if(lastAccBuffer.at(16+i) != ACDCFRAME && lastAccBuffer.at(16+i) != 0)
		{
			cout << "Board "<< i << " not with 32 words after ACC buffer read, ";
			cout << "Size " << lastAccBuffer.at(16+i)  << endl;
			retval = -1;
			continue;
		}else if(lastAccBuffer.at(16+i) != ACDCFRAME)
		{
			cout << "Board "<< i << " not with 32 words after ACC buffer read ";
			cout << "Size " << lastAccBuffer.at(16+i)  << endl;
			continue;
		}

		//both (1<<i) and (1<<i+8) should be true if aligned & synced respectively
		if((alignment_packet & (1 << i)))
		{
			//the i'th board is connected
			connectedBoards.push_back(i);
		}
	}
	if(connectedBoards.size()==0 || retval==-1)
	{
		return -1;
	}

	//this allows no vector clearing to be needed
	alignedAcdcIndices = connectedBoards;
	cout << "Connected Boards: " << alignedAcdcIndices.size() << endl;
	return 1;
}

/*ID 17: Main init function that controls generalk setup as well as trigger settings*/
int ACC::initializeForDataReadout(int trigMode, unsigned int boardMask, int calibMode)
{
	unsigned int command;
	int retval;
	
	// Creates ACDCs for readout
	retval = createAcdcs();
	if(retval==0)
	{
		writeErrorLog("ACDCs could not be created");
	}

	// Toogels the calibration mode on if requested
	toggleCal(calibMode, 0x7FFF, boardMask);

	// Set trigger conditions
	switch(trigMode)
	{ 	
		case 0: //OFF
			writeErrorLog("Trigger source turned off");	
			break;
		case 1: //Software trigger
			setSoftwareTrigger(boardMask);
			break;
		case 2: //SMA trigger ACC 
			setHardwareTrigSrc(trigMode,boardMask);
			command = 0x00310000;
			command = command | ACC_sign;
			usbcheck=usb->sendData(command); if(usbcheck==false){writeErrorLog("Send Error");}	
			break;
		case 3: //SMA trigger ACDC 
			setHardwareTrigSrc(trigMode,boardMask);
			command = 0x00B20000;
			command = (command | (boardMask << 24)) | ACDC_sign;
			usbcheck=usb->sendData(command); if(usbcheck==false){writeErrorLog("Send Error");}	
			break;
		case 4: //Self trigger
			setHardwareTrigSrc(trigMode,boardMask);
			goto selfsetup;

			break;				
		case 5: //Self trigger with SMA validation on ACC
 			setHardwareTrigSrc(trigMode,boardMask);
			command = 0x00310000;
			command = command | ACC_sign;
			usbcheck=usb->sendData(command); if(usbcheck==false){writeErrorLog("Send Error");}	

			command = 0x00320000;
			command = command | validation_start;
			usbcheck=usb->sendData(command); if(usbcheck==false){writeErrorLog("Send Error");}	
			command = 0x00330000;
			command = command | validation_window;
			usbcheck=usb->sendData(command); if(usbcheck==false){writeErrorLog("Send Error");}	
			command = 0x00350000;
			command = command | PPSBeamMultiplexer;
			usbcheck=usb->sendData(command); if(usbcheck==false){writeErrorLog("Send Error");}	
			goto selfsetup;
			break;
		case 6: //Self trigger with SMA validation on ACDC
			setHardwareTrigSrc(trigMode,boardMask);
			command = 0x00B20000;
			command = (command | (boardMask << 24)) | ACDC_sign;
			usbcheck=usb->sendData(command); if(usbcheck==false){writeErrorLog("Send Error");}	

			command = 0x00320000;
			command = command | validation_start;
			usbcheck=usb->sendData(command); if(usbcheck==false){writeErrorLog("Send Error");}	
			command = 0x00330000;
			command = command | validation_window;
			usbcheck=usb->sendData(command); if(usbcheck==false){writeErrorLog("Send Error");}	
			goto selfsetup;
			break;
		case 7:
			setHardwareTrigSrc(trigMode,boardMask);
			command = 0x00320000;
			command = command | validation_start;
			usbcheck=usb->sendData(command); if(usbcheck==false){writeErrorLog("Send Error");}	
			command = 0x00330000;
			command = command | validation_window;
			usbcheck=usb->sendData(command); if(usbcheck==false){writeErrorLog("Send Error");}	
			command = 0x00B20000;
			command = (command | (boardMask << 24)) | ACDC_sign;
			usbcheck=usb->sendData(command); if(usbcheck==false){writeErrorLog("Send Error");}	
			command = 0x00310000;
			command = command | ACC_sign;
			usbcheck=usb->sendData(command); if(usbcheck==false){writeErrorLog("Send Error");}	
			break;
		case 8:
			setHardwareTrigSrc(trigMode,boardMask);
			command = 0x00320000;
			command = command | validation_start;
			usbcheck=usb->sendData(command); if(usbcheck==false){writeErrorLog("Send Error");}	
			printf("st: 0x%08x\n",command);
			command = 0x00330000;
			command = command | validation_window;
			usbcheck=usb->sendData(command); if(usbcheck==false){writeErrorLog("Send Error");}	
			printf("win: 0x%08x\n",command);
			command = 0x00B20000;
			command = (command | (boardMask << 24)) | ACDC_sign;
			usbcheck=usb->sendData(command); if(usbcheck==false){writeErrorLog("Send Error");}	
			command = 0x00310000;
			command = command | ACC_sign;
			usbcheck=usb->sendData(command); if(usbcheck==false){writeErrorLog("Send Error");}	
			command = 0x00350000;
			command = command | PPSBeamMultiplexer;
			usbcheck=usb->sendData(command); if(usbcheck==false){writeErrorLog("Send Error");}	
			break;
		case 9: 
			setHardwareTrigSrc(trigMode,boardMask);
			break;
		default: // ERROR case
			writeErrorLog("Trigger source Error");	
			break;
		selfsetup:
 			command = 0x00B10000;
			if(SELF_psec_chip_mask.size()!=SELF_psec_channel_mask.size())
			{
				writeErrorLog("PSEC mask settings error");
			}
			for(int i=0; i<(int)SELF_psec_chip_mask.size(); i++)
			{	
				command = (command | (boardMask << 24)) | (SELF_psec_chip_mask[i] << 12) | SELF_psec_channel_mask[i];
				usbcheck=usb->sendData(command); if(usbcheck==false){writeErrorLog("Send Error");}	
			}
			command = 0x00B16000;
			command = (command | (boardMask << 24)) | SELF_sign;
			usbcheck=usb->sendData(command);	if(usbcheck==false){writeErrorLog("Send Error");}			
			command = 0x00B15000;
			command = (command | (boardMask << 24)) | SELF_number_channel_coincidence;
			usbcheck=usb->sendData(command); if(usbcheck==false){writeErrorLog("Send Error");}	
			command = 0x00B18000;
			command = (command | (boardMask << 24)) | SELF_coincidence_onoff;
			usbcheck=usb->sendData(command); if(usbcheck==false){writeErrorLog("Send Error");}	
			command = 0x00A60000;
			command = (command | (boardMask << 24)) | (0x1F << 12) | SELF_threshold;
			usbcheck=usb->sendData(command); if(usbcheck==false){writeErrorLog("Send Error");}	
	}

	command = 0x00340000;
	command = command | PPSRatio;
	printf("cmd: 0x%08x\n", command); 
	usbcheck=usb->sendData(command); if(usbcheck==false){writeErrorLog("Send Error");}	
	return 0;
}

/*ID 12: Set up the software trigger*/
void ACC::setSoftwareTrigger(unsigned int boardMask)
{	
	unsigned int command;

	//Set the trigger
	command = 0x00B00001; //Sets the trigger of all ACDC boards to 1 = Software trigger
	command = (command | (boardMask << 24)); 
	usbcheck=usb->sendData(command); if(usbcheck==false){writeErrorLog("Send Error");}
	command = 0x00300000; //Sets all ACDC boards to software trigger on the ACC 
	command = (command | (boardMask << 4)) | 1; 
	usbcheck=usb->sendData(command); if(usbcheck==false){writeErrorLog("Send Error");}
}

/*ID 21: Set up the hardware trigger*/
void ACC::setHardwareTrigSrc(int src, unsigned int boardMask)
{
	if(src > 9){
		string err_msg = "Source: ";
		err_msg += to_string(src);
		err_msg += " will cause an error for setting Hardware triggers. Source has to be <9";
		writeErrorLog(err_msg);
	}

	//ACC hardware trigger
	unsigned int command = 0x00300000;
	command = (command | (boardMask << 4)) | (unsigned short)src;
	usbcheck=usb->sendData(command); if(usbcheck==false){writeErrorLog("Send Error");}
	//ACDC hardware trigger
	command = 0x00B00000;
	command = (command | (boardMask << 24)) | (unsigned short)src;
	usbcheck=usb->sendData(command); if(usbcheck==false){writeErrorLog("Send Error");}
}

/*ID 20: Switch for the calibration input on the ACC*/
void ACC::toggleCal(int onoff, unsigned int channelmask, unsigned int boardMask)
{
	unsigned int command = 0x00C00000;
	//the firmware just uses the channel mask to toggle
	//switch lines. So if the cal is off, all channel lines
	//are set to be off. Else, uses channel mask
	if(onoff == 1)
	{
		//channelmas is default 0x7FFF
		command = (command | (boardMask << 24)) | channelmask;
	}else if(onoff == 0)
	{
		command = (command | (boardMask << 24));
	}
	usbcheck=usb->sendData(command); if(usbcheck==false){writeErrorLog("Send Error");}
}


/*------------------------------------------------------------------------------------*/
/*---------------------------Read functions listening for data------------------------*/

/*ID 14: Software read function*/
int ACC::readAcdcBuffers(bool raw, string timestamp)
{
	bool corruptBuffer;
	vector<int> boardsReadyForRead;
	map<int,int> readoutSize;
	unsigned int command;
	int maxCounter=0;
	bool clearCheck;

	//filename logistics
	string outfilename = "./Results/";
	string datafn;
	ofstream dataofs;

	//Enables the transfer of data from ACDC to ACC
   	enableTransfer(1);

	while(true)
	{
		boardsReadyForRead.clear();
		readoutSize.clear();
		//Request the ACC info frame to check buffers
		command = 0x00200000;
		usbcheck=usb->sendData(command);
		if(usbcheck==false)
		{
			writeErrorLog("Send Error");
			clearCheck = emptyUsbLine();
			if(clearCheck==false)
			{
				writeErrorLog("After failed send, emptying the USB lines failed as well");
			}
		}
		
		lastAccBuffer = usb->safeReadData(ACCFRAME);
		
		if(lastAccBuffer.size()==0)
		{
			std::cout << "ACCFRAME came up with " << lastAccBuffer.size() << std::endl;
			continue;
		}

		for(int k=0; k<MAX_NUM_BOARDS; k++)
		{
			if(lastAccBuffer.at(14) & (1 << k))
			{
				if(lastAccBuffer.at(16+k)==PSECFRAME)
				{
					boardsReadyForRead.push_back(k);
					readoutSize[k] = PSECFRAME;
				}else if(lastAccBuffer.at(16+k)==PPSFRAME)
				{
					boardsReadyForRead.push_back(k);
					readoutSize[k] = PPSFRAME;
				}
			}
		}

		//old trigger
		if(boardsReadyForRead==alignedAcdcIndices)
		{
			break;
		}

		/*new trigger
		std::sort(boardsReadyForRead.begin(), boardsReadyForRead.end());
		bool control = false;
		if(boardsReadyForRead.size()%2==0)
		{
			for(int m=0; m<boardsReadyForRead.size(); m+=2)
			{
				if({boardsReadyForRead[m],boardsReadyForRead[m+1]}=={0,1})
				{
					control = true;
				}else if({boardsReadyForRead[m],boardsReadyForRead[m+1]}=={2,3})
				{
					control = true;
				}else if({boardsReadyForRead[m],boardsReadyForRead[m+1]}=={4,5})
				{
					control = true;
				}else if({boardsReadyForRead[m],boardsReadyForRead[m+1]}=={6,7})
				{
					control = true;
				}else
				{
					control = false;
				}
			}
			if(control==true)
			{
				break;
			}
		}*/

		maxCounter++;
		if(maxCounter>500)
		{
			return 2;
		}
	}

	for(int bi: boardsReadyForRead)
	{
		//base command for set readmode and which board bi to read
		unsigned int command = 0x00210000; 
		command = command | (unsigned int)(bi); 
		usbcheck=usb->sendData(command); if(usbcheck==false){writeErrorLog("Send Error");}	

		//Tranfser the data to a receive vector
		vector<unsigned short> acdc_buffer = usb->safeReadData(readoutSize[bi]);

		//Handles buffers =/= 7795 words
		if((int)acdc_buffer.size() != readoutSize[bi])
		{
			string err_msg = "Couldn't read ";
			err_msg += to_string(readoutSize[bi]);
			err_msg += " words as expected! Tryingto fix it! Size was: ";
			err_msg += to_string(acdc_buffer.size());
			writeErrorLog(err_msg);
			return 1;
		}
		if(acdc_buffer[0] != 0x1234)
		{
			acdc_buffer.clear();
		}

		
		//save this buffer a private member of ACDC
		//by looping through our acdc vector
		//and checking each index 
		for(ACDC* a: acdcs)
		{
			if(a->getBoardIndex() == bi)
			{
				int retval;

				//If raw data is requested save and return 0
				if(raw==true && strcmp(timestamp,"NoSave")!=0)
				{
					vbuffer = acdc_buffer;
					string rawfn = outfilename + "Raw_" + timestamp + "_b" + to_string(bi) + ".txt";
					writeRawDataToFile(acdc_buffer, rawfn);
					break;
				}else
				{
					retval = a->parseDataFromBuffer(acdc_buffer); 
					corruptBuffer = meta.parseBuffer(acdc_buffer);
					if(corruptBuffer)
					{
						writeErrorLog("Metadata error not parsed correctly");
						return 1;
					}
					meta.checkAndInsert("Board", bi);
					map_meta[bi] = meta.getMetadata();
					if(retval !=0)
					{
						string err_msg = "Corrupt buffer caught at PSEC data level (2)";
						if(retval == 3)
						{
							err_msg += "Because of the Metadata buffer";
						}
						writeErrorLog(err_msg);
						corruptBuffer = true;
					}

					if(corruptBuffer)
					{
						string err_msg = "got a corrupt buffer with retval ";
						err_msg += to_string(retval);
						writeErrorLog(err_msg);
						return 1;
					}
	
					map_data[bi] = a->returnData();
				}
			}
		}
	}
	if(raw==false && strcmp(timestamp,"NoSave")!=0)
	{
		datafn = outfilename + "Data_" + timestamp + ".txt";
		dataofs.open(datafn.c_str(), ios::app);
		writePsecData(dataofs, boardsReadyForRead);
	}
/*
	command = 0xFFB54000;
	usbcheck=usb->sendData(command); if(usbcheck==false){writeErrorLog("Send Error");}	
	command = 0xFFD00000;
	usbcheck=usb->sendData(command); if(usbcheck==false){writeErrorLog("Send Error");}	

	for(int i: boardsReadyForRead)
	{
		command = 0x00210000;
		command = command | i;
		usbcheck=usb->sendData(command); if(usbcheck==false){writeErrorLog("Send Error");}	
		lastAccBuffer = usb->safeReadData(ACDCFRAME);
		if(lastAccBuffer.size()==ACDCFRAME)
		{
			if(lastAccBuffer.at(1)=0xbbbb)
			{
				map_acdcIF[i] = lastAccBuffer;
			}else
			{
				std::cout << "Board " << i << " got the wrong info frame" << std::endl;
			}
		}else
		{
			std::cout << "Board " << i << " got =/= 32 words!" << std::endl;
			map_acdcIF[i] = {0};
		}*/
	}

	return 0;
}

/*ID 15: Main listen fuction for data readout. Runs for 5s before retuning a negative*/
int ACC::listenForAcdcData(int trigMode, bool raw, string timestamp)
{
	bool corruptBuffer;
	vector<int> boardsReadyForRead;
	map<int,int> readoutSize;
	unsigned int command; 
	bool clearCheck;

	//filename logistics
	string outfilename = "./Results/";
	string datafn;
	ofstream dataofs;

	//this function is simply readAcdcBuffers
	//if the trigMode is software
	if(trigMode == 1)
	{
		int retval = readAcdcBuffers(raw, timestamp);
		return retval;
	}

	//setup a sigint capturer to safely
	//reset the boards if a ctrl-c signal is found
	struct sigaction sa;
	memset( &sa, 0, sizeof(sa) );
	sa.sa_handler = got_signal;
	sigfillset(&sa.sa_mask);
	sigaction(SIGINT,&sa,NULL);

	//Enables the transfer of data from ACDC to ACC
   	enableTransfer(1); 
  	
	//duration variables
	auto start = chrono::steady_clock::now(); //start of the current event listening. 
	auto now = chrono::steady_clock::now(); //just for initialization 
	auto printDuration = chrono::seconds(5); //prints as it loops and listens
	auto lastPrint = chrono::steady_clock::now();
	auto timeoutDuration = chrono::seconds(100); // will exit and reinitialize

	while(true)
	{ 
		//Clear the boards read vector
		boardsReadyForRead.clear(); 
		readoutSize.clear();
		//Time the listen fuction
		now = chrono::steady_clock::now();
		if(chrono::duration_cast<chrono::seconds>(now - lastPrint) > printDuration)
		{	
			string err_msg = "Have been waiting for a trigger for ";
			err_msg += to_string(chrono::duration_cast<chrono::seconds>(now - start).count());
			err_msg += " seconds";
			writeErrorLog(err_msg);
			for(int i=0; i<MAX_NUM_BOARDS; i++)
			{
				string err_msg = "Buffer for board ";
				err_msg += to_string(i);
				err_msg += " has ";
				err_msg += to_string(lastAccBuffer.at(16+i));
				err_msg += " words";
				writeErrorLog(err_msg);
			}
			lastPrint = chrono::steady_clock::now();
		}
		if(chrono::duration_cast<chrono::seconds>(now - start) > timeoutDuration)
		{
			return 2;
		}

		//If sigint happens, return value of 3
		if(quitacc.load())
		{
			return 3;
		}

		//Request the ACC info frame to check buffers
		command = 0x00200000;
		usbcheck=usb->sendData(command);
		if(usbcheck==false)
		{
			writeErrorLog("Send Error");
			clearCheck = emptyUsbLine();
			if(clearCheck==false)
			{
				writeErrorLog("After failed send, emptying the USB lines failed as well");
			}
		}
		
		lastAccBuffer = usb->safeReadData(ACCFRAME);
		
		if(lastAccBuffer.size()==0)
		{
			std::cout << "ACCFRAME came up with " << lastAccBuffer.size() << std::endl;
			continue;
		}

		//go through all boards on the acc info frame and if 7795 words were transfered note that board
		for(int k=0; k<MAX_NUM_BOARDS; k++)
		{
			if(lastAccBuffer.at(14) & (1 << k))
			{
				if(lastAccBuffer.at(16+k)==PSECFRAME)
				{
					boardsReadyForRead.push_back(k);
					readoutSize[k] = PSECFRAME;
				}else if(lastAccBuffer.at(16+k)==PPSFRAME)
				{
					boardsReadyForRead.push_back(k);
					readoutSize[k] = PPSFRAME;
				}
			}
		}

		//old trigger
		if(boardsReadyForRead==alignedAcdcIndices)
		{
			break;
		}

		/*new trigger
		std::sort(boardsReadyForRead.begin(), boardsReadyForRead.end());
		bool control = false;
		if(boardsReadyForRead.size()%2==0)
		{
			for(int m=0; m<boardsReadyForRead.size(); m+=2)
			{
				if({boardsReadyForRead[m],boardsReadyForRead[m+1]}=={0,1})
				{
					control = true;
				}else if({boardsReadyForRead[m],boardsReadyForRead[m+1]}=={2,3})
				{
					control = true;
				}else if({boardsReadyForRead[m],boardsReadyForRead[m+1]}=={4,5})
				{
					control = true;
				}else if({boardsReadyForRead[m],boardsReadyForRead[m+1]}=={6,7})
				{
					control = true;
				}else
				{
					control = false;
				}
			}
			if(control==true)
			{
				break;
			}
		}*/
	}

	//each ACDC needs to be queried individually
	//by the ACC for its buffer. 
	for(int bi: boardsReadyForRead)
	{
		//base command for set readmode and which board bi to read
		unsigned int command = 0x00210000; 
		command = command | (unsigned int)(bi); 
		usbcheck=usb->sendData(command); if(usbcheck==false){writeErrorLog("Send Error");}	

		//Tranfser the data to a receive vector
		vector<unsigned short> acdc_buffer = usb->safeReadData(readoutSize[bi]);

		//Handles buffers =/= 7795 words
		if((int)acdc_buffer.size() != readoutSize[bi])
		{
			string err_msg = "Couldn't read 7795 words as expected! Tryingto fix it! Size was: ";
			err_msg += to_string(acdc_buffer.size());
			writeErrorLog(err_msg);
			return 1;
		}
		if(acdc_buffer[0] != 0x1234)
		{
			acdc_buffer.clear();
		}


		//save this buffer a private member of ACDC
		//by looping through our acdc vector
		//and checking each index 
		for(ACDC* a: acdcs)
		{
			if(a->getBoardIndex() == bi)
			{
				int retval;

				//If raw data is requested save and return 0
				if(raw==true && strcmp(timestamp,"NoSave")!=0)
				{
					vbuffer = acdc_buffer;
					string rawfn = outfilename + "Raw_" + timestamp + "_b" + to_string(bi) + ".txt";
					writeRawDataToFile(acdc_buffer, rawfn);
					break;
				}else
				{
					//parśe raw data to channel data and metadata
					retval = a->parseDataFromBuffer(acdc_buffer); 
					meta.checkAndInsert("Board", bi);
					corruptBuffer = meta.parseBuffer(acdc_buffer);
					
					//check metadata for corrupt buffer
					if(corruptBuffer)
					{
						writeErrorLog("Metadata error not parsed correctly");
						return 1;
					}
					map_meta[bi] = meta.getMetadata();

					//check channel data for corrupt buffer
					if(retval !=0)
					{
						string err_msg = "Corrupt buffer caught at PSEC data level (2)";
						if(retval == 3)
						{
							err_msg += "Because of the Metadata buffer";
						}
						writeErrorLog(err_msg);
						corruptBuffer = true;
					}
					if(corruptBuffer)
					{
						string err_msg = "got a corrupt buffer with retval ";
						err_msg += to_string(retval);
						writeErrorLog(err_msg);
						return 1;
					}			
					map_data[bi] = a->returnData();
				}
			}
		}
	}
	if(raw==false && strcmp(timestamp,"NoSave")!=0)
	{
		datafn = outfilename + "Data_" + timestamp + ".txt";
		dataofs.open(datafn.c_str(), ios::app); 
		writePsecData(dataofs, boardsReadyForRead);
	}
/*
	command = 0xFFB54000;
	usbcheck=usb->sendData(command); if(usbcheck==false){writeErrorLog("Send Error");}	
	command = 0xFFD00000;
	usbcheck=usb->sendData(command); if(usbcheck==false){writeErrorLog("Send Error");}	

	for(int i: boardsReadyForRead)
	{
		command = 0x00210000;
		command = command | i;
		usbcheck=usb->sendData(command); if(usbcheck==false){writeErrorLog("Send Error");}	
		lastAccBuffer = usb->safeReadData(ACDCFRAME);
		if(lastAccBuffer.size()==ACDCFRAME)
		{
			if(lastAccBuffer.at(1)=0xbbbb)
			{
				map_acdcIF[i] = lastAccBuffer;
			}else
			{
				std::cout << "Board " << i << " got the wrong info frame" << std::endl;
			}
		}else
		{
			std::cout << "Board " << i << " got =/= 32 words!" << std::endl;
			map_acdcIF[i] = {0};
		}*/
	}

	return 0;
}


/*------------------------------------------------------------------------------------*/
/*---------------------------Active functions for informations------------------------*/

/*ID 19: Pedestal setting procedure.*/
bool ACC::setPedestals(unsigned int boardmask, unsigned int chipmask, unsigned int adc)
{
	unsigned int command = 0x00A20000;
	command = (command | (boardmask << 24)) | (chipmask << 12) | adc;
	usbcheck=usb->sendData(command); if(usbcheck==false){writeErrorLog("Send Error");}
	return true;
}

/*ID 24: Special function to check connected ACDCs for their firmware version*/ 
void ACC::versionCheck()
{
	unsigned int command;
	
	//Request ACC info frame
	command = 0x00200000; 
	usb->sendData(command);
	
	lastAccBuffer = usb->safeReadData(SAFE_BUFFERSIZE);
	if(lastAccBuffer.size()==ACCFRAME)
	{
		if(lastAccBuffer.at(1)=0xaaaa)
		{
			std::cout << "ACC got the firmware version: " << std::hex << lastAccBuffer.at(2) << std::dec;
			std::cout << " from " << std::hex << lastAccBuffer.at(4) << std::dec << "/" << std::hex << lastAccBuffer.at(3) << std::dec << std::endl;
		}else
		{
			std::cout << "ACC got the wrong info frame" << std::endl;
		}
	}else
	{
		std::cout << "ACC got the no info frame" << std::endl;
	}

	//Disables Psec communication
	command = 0xFFB54000; 
	usb->sendData(command);

	//Give the firmware time to disable
	usleep(10000); 

	//Read the ACC infoframe, use sendAndRead for additional buffersize to prevent
	//leftover words
	command = 0xFFD00000; 
	usb->sendData(command);

	//Loop over the ACC buffer words that show the ACDC buffer size
	//32 words represent a connected ACDC
	for(int i = 0; i < MAX_NUM_BOARDS; i++)
	{
		command = 0x00210000;
		command = command | i;
		usb->sendData(command);

		lastAccBuffer = usb->safeReadData(SAFE_BUFFERSIZE);
		if(lastAccBuffer.size()==ACDCFRAME)
		{
			if(lastAccBuffer.at(1)=0xbbbb)
			{
				std::cout << "Board " << i << " got the firmware version: " << std::hex << lastAccBuffer.at(2) << std::dec;
				std::cout << " from " << std::hex << lastAccBuffer.at(4) << std::dec << "/" << std::hex << lastAccBuffer.at(3) << std::dec << std::endl;
			}else
			{
				std::cout << "Board " << i << " got the wrong info frame" << std::endl;
			}
		}else
		{
			std::cout << "Board " << i << " is not connected" << std::endl;
		}
	}
}


/*------------------------------------------------------------------------------------*/
/*-------------------------------------Help functions---------------------------------*/

/*ID 13: Fires the software trigger*/
void ACC::softwareTrigger()
{
	//Software trigger
	unsigned int command = 0x00100000;
	usbcheck=usb->sendData(command); if(usbcheck==false){writeErrorLog("Send Error");}	
}

/*ID 16: Used to dis/enable transfer data from the PSEC chips to the buffers*/
void ACC::enableTransfer(int onoff)
{
	unsigned int command;
	if(onoff == 0)//OFF
	{ 
		command = 0xFFB54000;
		usbcheck=usb->sendData(command); if(usbcheck==false){writeErrorLog("Send Error");}	
	}else if(onoff == 1)//ON
	{
		command = 0xFFB50000;
		usbcheck=usb->sendData(command); if(usbcheck==false){writeErrorLog("Send Error");}	
	}
}

/*ID 7: Function to completly empty the USB line until the correct response is received*/
bool ACC::emptyUsbLine()
{
	int send_counter = 0; //number of usb sends
	int max_sends = 10; //arbitrary. 
	unsigned int command = 0x00200000; // a refreshing command
	vector<unsigned short> tempbuff;
	while(true)
	{
		usb->safeReadData(SAFE_BUFFERSIZE);
		usbcheck=usb->sendData(command); if(usbcheck==false){writeErrorLog("Send Error");}
		send_counter++;
		tempbuff = usb->safeReadData(SAFE_BUFFERSIZE);

		//if it is exactly an ACC buffer size, success. 
		if(tempbuff.size() == ACCFRAME)
		{
			return true;
		}
		if(send_counter > max_sends)
		{
			string err_msg = "Something wrong with USB line, waking it up. got ";
			err_msg += to_string(tempbuff.size());
			err_msg += " words";
			writeErrorLog(err_msg);
			usbWakeup();
			tempbuff = usb->safeReadData(SAFE_BUFFERSIZE);
			if(tempbuff.size() == ACCFRAME){
				return true;
			}else{
				writeErrorLog("Usb still sleeping. Problem is not fixed.");
				return false;
 			}
		}
	}
}

/*ID 8: USB return*/
stdUSB* ACC::getUsbStream()
{
	return usb;
}

/*ID 10: Clear all ACDC class instances*/
void ACC::clearAcdcs()
{
	for(int i = 0; i < (int)acdcs.size(); i++)
	{
		delete acdcs[i];
	}
	acdcs.clear();
}

/*ID 23: Wakes up the USB by requesting an ACC info frame*/
void ACC::usbWakeup()
{
	unsigned int command = 0x00200000;
	usbcheck=usb->sendData(command); if(usbcheck==false){writeErrorLog("Send Error");}
}

/*ID 18: Tells ACDCs to clear their ram.*/ 
void ACC::dumpData(unsigned int boardMask)
{
	unsigned int command = 0x00020000; //base command for set readmode
	command = command | boardMask;
	//send and read. 
	usbcheck=usb->sendData(command); if(usbcheck==false){writeErrorLog("Send Error");}
	//usb->safeReadData(SAFE_BUFFERSIZE + 2);
}

/*ID 26: Read ACC buffer for Info frame*/
vector<unsigned short> ACC::getACCInfoFrame()
{
	unsigned int command = 0x00200000;	 
	vector<unsigned short> buffer;
	int counter = 0;
	while(counter<5)
	{	
		usbcheck=usb->sendData(command); if(usbcheck==false){writeErrorLog("Send Error");}
		buffer = usb->safeReadData(SAFE_BUFFERSIZE);
		
		if(buffer.size()!=ACCFRAME)
		{
			counter++;
			writeErrorLog("Could not read ACC info frame");
		}else
		{
			map_accIF = buffer;
			return buffer;	
		}
	}
	return {};
}

/*ID 27: Resets the ACDCs*/
void ACC::resetACDC()
{
		unsigned int command = 0xFFFF0000;
		usb->sendData(command); if(usbcheck==false){writeErrorLog("Send Error");}
		usleep(1000000);
}

/*ID 28: Resets the ACCs*/
void ACC::resetACC()
{
		unsigned int command = 0x00000000;
		usb->sendData(command); if(usbcheck==false){writeErrorLog("Send Error");}
		usleep(1000000);
}


/*------------------------------------------------------------------------------------*/
/*-------------------------------------Help functions---------------------------------*/

/*ID 29: Write function for the error log*/
void ACC::writeErrorLog(string errorMsg)
{
    string err = "errorlog.txt";
    cout << "------------------------------------------------------------" << endl;
    cout << errorMsg << endl;
    cout << "------------------------------------------------------------" << endl;
    ofstream os_err(err, ios_base::app);
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%m-%d-%Y %X");
    os_err << "------------------------------------------------------------" << endl;
    os_err << ss.str() << endl;
    os_err << errorMsg << endl;
    os_err << "------------------------------------------------------------" << endl;
    os_err.close();
}

/*ID 30: Write function for the raw data format*/
void ACC::writeRawDataToFile(vector<unsigned short> buffer, string rawfn)
{
	ofstream d(rawfn.c_str(), ios::app); 
	for(unsigned short k: buffer)
	{
		d << hex <<  k << " ";
	}
	d << endl;
	d.close();
	return;
}

/*ID 31: Write function for the parsed data format*/
void ACC::writePsecData(ofstream& d, vector<int> boardsReadyForRead)
{
	vector<string> keys;
	//vector<string> lines;
	map<string, unsigned short> extra_key;
	for(int bi: boardsReadyForRead)
	{
		extra_key = map_meta[bi];
		if(extra_key.size()!=0)
		{
			break;
		}
	}

	for(auto const& element : extra_key) {
		keys.push_back(element.first);
	}

	string delim = " ";
	for(int enm=0; enm<NUM_SAMP; enm++)
	{
		d << dec << enm << delim;
		for(int bi: boardsReadyForRead)
		{
			if(map_data[bi].size()==0)
			{
				cout << "Mapdata is empty" << endl;
				writeErrorLog("Mapdata empty");
			}
			for(int ch=0; ch<NUM_CH; ch++)
			{
				if(enm==0)
				{
					//cout << "Writing board " << bi << " and ch " << ch << ": " << map_data[bi][ch+1][enm] << endl;
				}
				d << dec << (unsigned short)map_data[bi][ch+1][enm] << delim;
			}
			if(enm<(int)keys.size())
			{
				d << hex << map_meta[bi][keys[enm]] << delim;

			}else
			{
				d << 0 << delim;
			}
		}
		d << endl;
	}
	d.close();
}
