#include "Factory.h"

Tool* Factory(std::string tool) {
Tool* ret=0;

// if (tool=="Type") tool=new Type;
if (tool=="ListenForData") ret=new ListenForData;
if (tool=="Receive") ret=new Receive;
if (tool=="SetupBoards") ret=new SetupBoards;
if (tool=="Stream") ret=new Stream;
return ret;
}
