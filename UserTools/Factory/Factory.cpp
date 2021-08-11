#include "Factory.h"

Tool* Factory(std::string tool) {
Tool* ret=0;

// if (tool=="Type") tool=new Type;
if (tool=="ListenForData") ret=new ListenForData;
if (tool=="ParseData") ret=new ParseData;
if (tool=="SaveEvent") ret=new SaveEvent;
if (tool=="SetupBoards") ret=new SetupBoards;
return ret;
}
