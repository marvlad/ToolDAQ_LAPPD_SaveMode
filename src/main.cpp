#include <string>
#include "ToolChain.h"

int main(int argc, char* argv[]){

  std::string conffile;
  if (argc==1)conffile="configfiles/ReadOutChain/ToolChainConfig";
  else conffile=argv[1];

  ToolChain tools(conffile, argc, argv);

  //int portnum=24000;
  //  tools.Remote(portnum);
  //tools.Interactive();
   
  return 0;
  
}
