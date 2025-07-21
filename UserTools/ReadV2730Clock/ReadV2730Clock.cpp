#include "ReadV2730Clock.h"

ReadV2730Clock::ReadV2730Clock():Tool(){}


bool ReadV2730Clock::Initialise(std::string configfile, DataModel &data){
  
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

  handle = ReadV2730Clock::OpenBoard(m_variables);
  ReadV2730Clock::handle = handle;
  //std::cout<<"Board 1 handle: "<<handle<<std::endl;

  m_variables.Get("bID", bID);
  ReadV2730Clock::bID = bID;


  //ExportConfiguration();

  return true;
}


bool ReadV2730Clock::Execute(){

  uint64_t handle=ReadV2730Clock::handle;
  int ret; bID=ReadV2730Clock::bID;
  float delay;
  char value[256];

  m_variables.Get("delay", delay);
  std::cout<<"Delay from file: "<<delay<<std::endl;

  ret = ReadV2730Clock::SetFloatValue(handle, "/par/VolatileClockOutDelay", delay);
  ret = CAEN_FELib_GetValue(handle, "/par/VolatileClockOutDelay", value);
  if (ret) {
    std::cout<<"Error setting board "<<bID<<" volatile clock delay: "<<ret<<std::endl;
    return false;
  }
  else {
    std::cout<<"Set board "<<bID<<" volatile clock delay to: :<<value<<std::endl;
  }

  ret = CAEN_FELib_SetValue(handle, "/par/EnClockOutFP", "True");
  if (ret) {
    std::cout<<"Error setting board "<<bID<<" clock out enable: "<<ret<<std::endl;
    return false;
  }
  ret = CAEN_FELib_GetValue(handle, "/par/EnClockOutFP", value);
  if (verbose) {
    std::cout<<"Board "<<bID<<" clock out enable set to: "<<value<<std::endl;
  }


  ret = CAEN_FELib_SetValue(handle, "/par/TrgOutMode", "RefClk");
  if (ret) {
    std::cout<<"Error setting board "<<bID<<" TRGOUT mode: "<<ret<<std::endl;
    return false;
  }
  ret = CAEN_FELib_GetValue(handle, "/par/TrgOutMode", value);
  if (verbose) {
    std::cout<<"Board "<<bID<<" TRGOUT mode set to: "<<value<<std::endl;
  }

  return true;
}


bool ReadV2730Clock::Finalise(){

  int ret;
  uint64_t handle = ReadV2730Clock::handle;

  ret = CAEN_FELib_Close(handle);
  std::cout<<"Close Board "<<ReadV2730Clock::bID<<": "<<ret<<std::endl;

  return true;
}

// Opens board and returns board handle
uint64_t ReadV2730Clock::OpenBoard(Store m_variables){

  //uint32_t address;
  uint64_t handle;
  int bID, VME_bridge, LinkNum, ConetNode, verbose;
  int ret;
  char model[256];
  std::string path;

  //m_variables.Get("VME_bridge", VME_bridge);
  //m_variables.Get("LinkNum", LinkNum);
  //m_variables.Get("ConetNode", ConetNode);
  m_variables.Get("verbose", verbose);
  m_variables.Get("bID", bID);
  m_variables.Get("path", path);
/*
  if (VME_bridge) {

    std::string tmp;

    m_variables.Get("address", tmp);
    address = std::stoi(tmp, 0, 16);
  }
  else address = 0;
*/
  std::string url = std::string("dig2://caen.internal") + path;
                                 //std::to_string(LinkNum) + "&conet_node=" + std::to_string(ConetNode) + 
                                 //"&vme_base_address=" + std::to_string(address);
  const char *url_cstr = url.c_str();
  ret = CAEN_FELib_Open(url_cstr, &handle);
  if (ret) std::cout<<"Error opening board "<<bID<<". CAEN FELib Error Code: "<<ret<<std::endl;

  ret = CAEN_FELib_GetValue(handle, "/par/ModelName", model);
  if (ret) std::cout <<"Error getting board "<<bID<<" info. CAEN FELib Error Code: "<<ret<<std::endl;
  else if (!ret && verbose) {
    std::cout<<"Connected to Board "<<bID<<" Model: "<<model<<std::endl;
  }
  ReadV2730Clock::model = std::string(model);
/*
  ret = CAEN_FELib_GetValue(handle, "/par/FwType", model);
  std::cout<<"Firmware Type: "<<model<<std::endl;
  ret = CAEN_FELib_GetValue(handle, "/par/FPGA_FwVer", model);
  std::cout<<"Firmware Version: "<<model<<std::endl;
*/
  return handle;
}

int ReadV2730Clock::SetFloatValue(uint64_t handle, const char *path, float val) {
  char value[256];
  snprintf(value, sizeof(value), "%f", val);
  int ret = CAEN_FELib_SetValue(handle, path, value);
  return ret;
}
