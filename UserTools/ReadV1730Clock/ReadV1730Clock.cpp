#include "ReadV1730Clock.h"

ReadV1730Clock::ReadV1730Clock():Tool(){}


bool ReadV1730Clock::Initialise(std::string configfile, DataModel &data){
  
  //InitialiseTool(data);
  //InitialiseConfiguration(configfile);
  //m_variables.Print();

  if(configfile!="")  m_variables.Initialise(configfile);

  m_data= &data;
  m_log= m_data->Log;

  if(!m_variables.Get("verbose",m_verbose)) m_verbose=1;

  uint64_t handle;
  int bID;
  int ret;

  handle = ReadV1730Clock::OpenBoard(m_variables);
  ReadV1730Clock::handle = handle;
  //std::cout<<"Board 1 handle: "<<handle<<std::endl;

  m_variables.Get("bID", bID);
  ReadV1730Clock::bID = bID;

  //ExportConfiguration();

  return true;
}


bool ReadV1730Clock::Execute(){

  int handle=ReadV1730Clock::handle, bID=ReadV1730Clock::bID;
  int ret;

  ret = CAEN_FELib_SetUserRegister(handle, 0x811C, 0x00050000);
  if (!ret) std::cout<<"Board "<<bID<<" CLKOUT propagated to TRGOUT"<<std::endl;
  else std::cout<<ret<<std::endl;

  return true;
}


bool ReadV1730Clock::Finalise(){

  int ret;
  uint64_t handle = ReadV1730Clock::handle;

  ret = CAEN_FELib_Close(handle);
  std::cout<<"Close Board "<<ReadV1730Clock::bID<<": "<<ret<<std::endl;

  return true;
}

// Opens board and returns board handle
uint64_t ReadV1730Clock::OpenBoard(Store m_variables){

  uint32_t address;
  uint64_t handle;
  int bID, VME_bridge, LinkNum, ConetNode, verbose;
  int ret;
  char model[256];

  m_variables.Get("VME_bridge", VME_bridge);
  m_variables.Get("LinkNum", LinkNum);
  m_variables.Get("ConetNode", ConetNode);
  m_variables.Get("verbose", verbose);
  m_variables.Get("bID", bID);

  if (VME_bridge) {

    std::string tmp;

    m_variables.Get("address", tmp);
    address = std::stoi(tmp, 0, 16);
  }
  else address = 0;

  std::string url = std::string("dig1://caen.internal/optical_link?") + "link_num=" + 
                                 std::to_string(LinkNum) + "&conet_node=" + std::to_string(ConetNode) + 
                                 "&vme_base_address=" + std::to_string(address);
  const char *url_cstr = url.c_str();
  ret = CAEN_FELib_Open(url_cstr, &handle);
  if (ret) std::cout<<"Error opening digitizer. CAEN FELib Error Code: "<<ret<<std::endl;

  ret = CAEN_FELib_GetValue(handle, "/par/MODELNAME", model);
  if (ret) std::cout <<"Error getting board info. CAEN FELib Error Code: "<<ret<<std::endl;
  else if (!ret && verbose) {
    std::cout<<"Connected to Board "<<bID<<" Model: "<<model<<std::endl;
  }
  ReadV1730Clock::model = std::string(model);

  return handle;
}
/*
int ReadV1730Clock::SetFloatValue(uint64_t handle, const char *path, float val) {
  char value[256];
  snprintf(value, sizeof(value), "%f", val);
  int ret = CAEN_FELib_SetValue(handle, path, value);
  return ret;
}
*/
