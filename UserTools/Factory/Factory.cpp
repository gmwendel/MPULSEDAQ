#include "Factory.h"
#include "Unity.h"

Tool* Factory(std::string tool) {
Tool* ret=0;

// if (tool=="Type") tool=new Type;
if (tool=="ReadV1730") ret=new ReadV1730;
if (tool=="ReadV2730") ret=new ReadV2730;
return ret;
}
