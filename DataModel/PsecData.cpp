#include <PsecData.h>

PsecData::PsecData()
{
    VersionNumber = 0xA002;
    counter = 0;
    DataSave = 0;
    FailedReadCounter=0;
}

bool PsecData::Print(){
	std::cout << "--------------------Data Control Window-------------------" << std::endl;
    std::cout << "----------------------------------------------------------" << std::endl;
	printf("Version number: 0x%04x\n", VersionNumber);
	for(int i=0; i<BoardIndex.size(); i++)
	{
		printf("Board number: %i\n", BoardIndex[i]);
	}
	printf("Failed read attempts: %i\n", FailedReadCounter);
	printf("Waveform size: %li\n", RawWaveform.size());
	printf("ACC Infoframe size: %li\n", AccInfoFrame.size());
	std::cout << "----------------------------------------------------------" << std::endl;
	
	return true;
}
