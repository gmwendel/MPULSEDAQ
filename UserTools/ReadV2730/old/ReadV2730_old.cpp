#include "ReadV2730.h"

ReadV2730::ReadV2730():Tool(){}


bool ReadV2730::Initialise(std::string configfile, DataModel &data){

  if(configfile!="")  m_variables.Initialise(configfile);
  //m_variables.Print();

  m_data= &data;
  m_log= m_data->Log;

  if(!m_variables.Get("verbose",m_verbose)) m_verbose=1;

  return true;
}


bool ReadV2730::Execute(){

  uint64_t handle;
  //const char *url = "dig2://caendgtz-usb-52480";
  const char *url = "dig2://caen.internal/usb/52480";
  std::cout<<url<<std::endl;
  int ret = CAEN_FELib_Open(url, &handle);
  std::cout<<ret<<std::endl;

  ret = CAEN_FELib_Close(handle);
  std::cout<<ret<<std::endl;

  return true;
}


bool ReadV2730::Finalise(){

  return true;
}
