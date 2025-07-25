#include "ReadV2730.h"

ReadV2730::ReadV2730():Tool(){}


bool ReadV2730::Initialise(std::string configfile, DataModel &data){

  if(configfile!="")  m_variables.Initialise(configfile);
  //m_variables.Print();

  m_data= &data;
  m_log= m_data->Log;

  if(!m_variables.Get("verbose",m_verbose)) m_verbose=1;

  uint64_t handle;
  int bID;
  int ret;
  char value[256];

  handle = ReadV2730::OpenBoard(m_variables);
  ReadV2730::handle = handle;
  //std::cout<<"Board 1 handle: "<<handle<<std::endl;

  m_variables.Get("bID", bID);
  ReadV2730::bID = bID;

  ret = CAEN_FELib_SetValue(handle, "/par/EnClockOutFP", "True");
  if (ret) {
    std::cout<<"Error setting board "<<bID<<" clock out enable: "<<ret<<std::endl;
    return false;
  }
  ret = CAEN_FELib_GetValue(handle, "/par/EnClockOutFP", value);
  std::cout<<"Board "<<bID<<" clock out enable set to: "<<value<<std::endl;

  bool test = ReadV2730::ConfigureBoard(handle, m_variables);
  if(!test) {
    std::cout<<"Board "<<bID<<" configuration failed!!!"<<std::endl;
    return true;
  }
  else {
    std::cout<<"Board "<<bID<<" configured!"<<std::endl;
  }

  ReadV2730::evt = ReadV2730::allocate_event(ReadV2730::nchan, ReadV2730::nsamp);
  if (evt == NULL) {
    std::cout<<"Error allocating event"<<std::endl;
    return false;
  }

  int ev_per_file;
  m_variables.Get("ev_per_file", ev_per_file);
  ReadV2730::ev_per_file = ev_per_file;

  std::string ofile;
  m_variables.Get("ofile", ofile);

  std::string timestamp = ReadV2730::Get_TimeStamp();
  std::string ofile_part;
  ofile_part = ofile + "_" + timestamp + "_board" + std::to_string(bID) + "_";
  ReadV2730::ofile_part = ofile_part;
  std::string ofile_full = ofile_part + "0.h5";

  ReadV2730::outfile = ReadV2730::OpenOutFile(ofile_full, ReadV2730::ev_per_file, ReadV2730::nchan, ReadV2730::nsamp);

  return true;
}


bool ReadV2730::Execute(){
/*
  uint64_t handle;
  const char *url = "dig1://caen.internal/optical_link?link_num=0&conet_node=0&vme_base_address=0";
  std::cout<<url<<std::endl;
  int ret = CAEN_FELib_Open(url, &handle);
  std::cout<<"Open V1730: "<<ret<<std::endl;
*/
  int ret, Nb=ReadV2730::Nb, Ne=ReadV2730::Ne;
  uint64_t handle=ReadV2730::handle, ep_handle, CurrentTime, ElapsedRateTime, ElapsedSWTrigTime;

  if (!ReadV2730::acq_started) {
    ret = CAEN_FELib_SetValue(handle, "/endpoint/par/activeendpoint", "Scope");
    if (ret) {
      std::cout<<"Error setting board "<<ReadV2730::bID<<" active endpoint: "<<ret<<std::endl;
      return false;
    }

    ret = CAEN_FELib_GetHandle(handle, "/endpoint/scope", &ep_handle);
    if (ret) {
      std::cout<<"Error getting board "<<ReadV2730::bID<<" endpoint handle: "<<ret<<std::endl;
      return false;
    }
    ReadV2730::ep_handle = ep_handle;
    ret = CAEN_FELib_SetReadDataFormat(ep_handle, DATA_FORMAT_2730);
    if (ret) {
      std::cout<<"Error setting board "<<ReadV2730::bID<<" read data format: "<<ret<<std::endl;
      return false;
    }

    ret = CAEN_FELib_SendCommand(handle, "/cmd/ClearData");
    if (ret) {
      std::cout<<"Error clearing board "<<ReadV2730::bID<<" data: "<<ret<<std::endl;
      return false;
    }

    ret = CAEN_FELib_SendCommand(handle, "/cmd/ArmAcquisition");
    ret = CAEN_FELib_SendCommand(handle, "/cmd/SwStartAcquisition");
    if (!ret) {
      std::cout<<"Board "<<ReadV2730::bID<<" acquisition started"<<std::endl;
      ReadV2730::acq_started = 1;
    }
    else {
      std::cout<<"Error starting board "<<ReadV2730::bID<<" acquisition: "<<ret<<std::endl;
      return false;
    }

    ReadV2730::Nb=0;
    ReadV2730::Ne=0;
    ReadV2730::PrevRateTime = ReadV2730::get_time();
    ReadV2730::PrevSWTrigTime = ReadV2730::get_time();
    ReadV2730::event_count=0;

    float swtrigrate;
    m_variables.Get("SWTrigRate", swtrigrate);
    ReadV2730::swtrigrate = 1000/swtrigrate;

    /*
    char value[256];
    ret = CAEN_FELib_GetValue(handle, "/par/AcqTriggerSource", value);
    std::cout<<value<<std::endl;
    ret = CAEN_FELib_GetValue(handle, "/par/TrgOutMode", value);
    std::cout<<value<<std::endl;
    ret = CAEN_FELib_GetValue(handle, "/par/AcquisitionStatus", value);
    std::cout<<value<<std::endl;
    */
  }
  else {
    CurrentTime = ReadV2730::get_time();
    ElapsedRateTime = CurrentTime - ReadV2730::PrevRateTime;
    ElapsedSWTrigTime = CurrentTime - ReadV2730::PrevSWTrigTime;

    if (ElapsedSWTrigTime > (ReadV2730::swtrigrate)) {
      ret = CAEN_FELib_SendCommand(handle, "/cmd/SendSWTrigger");
//      std::cout<<ret<<std::endl;
      ReadV2730::PrevSWTrigTime = ReadV2730::get_time();
    }

    ret = CAEN_FELib_ReadData(ReadV2730::ep_handle, 0,
          &ReadV2730::evt->timestamp,
          &ReadV2730::evt->timestamp_ns,
          &ReadV2730::evt->trigger_id,
          ReadV2730::evt->waveform,
          ReadV2730::evt->waveform_size,
          &ReadV2730::evt->flags,
          &ReadV2730::evt->board_fail,
          &ReadV2730::evt->event_size
    );
    //if (ret != -11) std::cout<<ret<<std::endl;
    if (!ret) {
      Ne++;
      Nb += ReadV2730::evt->event_size;
      ReadV2730::event_count++;

      ReadV2730::WriteEvent(ReadV2730::outfile, ReadV2730::evt, ReadV2730::event_count - 1, ReadV2730::nchan);
    }

    if (ReadV2730::event_count == ReadV2730::ev_per_file) {
      ReadV2730::outfile.close();
      ReadV2730::file_num++;
      std::string fname = ReadV2730::ofile_part + std::to_string(ReadV2730::file_num) + ".h5";
      ReadV2730::outfile = ReadV2730::OpenOutFile(fname, ReadV2730::ev_per_file, ReadV2730::nchan, ReadV2730::nsamp);
      ReadV2730::event_count = 0;
    }

    int show_data_rate;
    m_variables.Get("show_data_rate", show_data_rate);
    if ((ElapsedRateTime > 2000) && show_data_rate) {
      if (Nb==0) std::cout<<"Board "<<ReadV2730::bID<<": No data..."<<std::endl;
      else {
        std::cout<<"Data rate Board "<<ReadV2730::bID<<": "<<(float)Nb/((float)ElapsedRateTime*1048.567f)<<" MB/s  Trigger Rate Board "<<ReadV2730::bID<<": "<<((float)Ne*1000.0f)/(float)ElapsedRateTime<<" Hz"<<std::endl;
      }

      Nb = 0;
      Ne = 0;
      ReadV2730::PrevRateTime = ReadV2730::get_time();
    }
  ReadV2730::Nb = Nb;
  ReadV2730::Ne = Ne;
  }

  return true;
}


bool ReadV2730::Finalise(){
  int ret, timeout=0;
  uint64_t handle = ReadV2730::handle;

  ret = CAEN_FELib_SendCommand(handle, "/cmd/disarmacquisition");
  if (!ret) {
    std::cout<<"Board "<<ReadV2730::bID<<" acquisition stopped"<<std::endl;
    ReadV2730::acq_started = 0;
  }
  else {
    std::cout<<"Error stopping board "<<ReadV2730::bID<<" acquisition: "<<ret<<std::endl;
    return false;
  }

  while(!timeout) {
    ret = CAEN_FELib_ReadData(ReadV2730::ep_handle, 1000,
          &ReadV2730::evt->timestamp,
          &ReadV2730::evt->timestamp_ns,
          &ReadV2730::evt->trigger_id,
          ReadV2730::evt->waveform,
          ReadV2730::evt->waveform_size,
          &ReadV2730::evt->flags,
          &ReadV2730::evt->board_fail,
          &ReadV2730::evt->event_size
    );
    if ((ret==0) || (ret==-11) || (ret==-12)) {
      timeout = ret;
    }
    else {
      std::cout<<"Error during board "<<ReadV2730::bID<<" ReadData: "<<ret<<std::endl;
      return false;
    }
  }

  //std::cout<<"Board 1 handle2: "<<handle<<std::endl;
  ret = CAEN_FELib_Close(handle);
  std::cout<<"Close Board "<<ReadV2730::bID<<": "<<ret<<std::endl;

  ReadV2730::free_event(ReadV2730::evt, ReadV2730::nchan);

  return true;
}

//===========================================================================================

// Gets current time in milliseconds
long ReadV2730::get_time(){

  long time_ms;

  struct timeval tv;
  struct timezone tz;

  gettimeofday(&tv, &tz);

  time_ms = tv.tv_sec * 1000 + tv.tv_usec / 1000;

  return time_ms;
}

// Gets timestamp as a string (DDMMYYYYTHHMM)
std::string ReadV2730::Get_TimeStamp(){

  char timestamp[13];

  time_t rawtime;
  struct tm* timeinfo;

  time(&rawtime);
  timeinfo = localtime(&rawtime);
  strftime(timestamp, 13, "%y%m%dT%H%M", timeinfo);

  return timestamp;
}

// Opens board and returns board handle
uint64_t ReadV2730::OpenBoard(Store m_variables){

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
  ReadV2730::model = std::string(model);
/*
  ret = CAEN_FELib_GetValue(handle, "/par/FwType", model);
  std::cout<<"Firmware Type: "<<model<<std::endl;
  ret = CAEN_FELib_GetValue(handle, "/par/FPGA_FwVer", model);
  std::cout<<"Firmware Version: "<<model<<std::endl;
*/
  return handle;
}

int ReadV2730::SetFloatValue(uint64_t handle, const char *path, float val) {
  char value[256];
  snprintf(value, sizeof(value), "%f", val);
  int ret = CAEN_FELib_SetValue(handle, path, value);
  return ret;
}

// Sets board settings like trigger threshold, record length, DC offset, etc.
bool ReadV2730::ConfigureBoard(uint64_t handle, Store m_variables) {

  std::string bdname = ReadV2730::model;
  std::string tmp, tmp2, GrpSelfTrigMode;
  uint32_t ChanEnableMask, ChanSelfTrigMask, GrpEnableMask, GrpSelfTrigMask;
  int ChanSelfTrigEnable, ChanSelfTrigOut;

  if (bdname.find("730") != std::string::npos) {
    m_variables.Get("ChanEnableMask", tmp);
    m_variables.Get("ChanSelfTrigMask", tmp2);
    m_variables.Get("ChanSelfTrigEnable", ChanSelfTrigEnable);
    m_variables.Get("ChanSelfTrigOut", ChanSelfTrigOut);
    ChanEnableMask = std::stol(tmp, 0, 16);
    ChanSelfTrigMask = std::stol(tmp2, 0, 16);
  }

  int thresh;
  uint32_t length, percent, reg;
  uint16_t dec_factor;
  int bID, verbose, use_ETTT, TrigInEnable, SWTrigEnable, ITLComboEnable, TrigInOut, SWTrigOut, AcceptedTrigOut;
  int recLen, preTrig, mask=0;
  float dynRange, DCOff;
  char value[256];
  std::string polarity, IOLevel;
  //CAEN_DGTZ_TriggerPolarity_t pol;
  //CAEN_DGTZ_TriggerMode_t selftrigmode;
  //CAEN_DGTZ_IOLevel_t iolevel;
  //CAEN_DGTZ_ErrorCode ret;
  int ret;

  m_variables.Get("RecLen", recLen);
  m_variables.Get("PreTrig", preTrig);
  m_variables.Get("DynRange", dynRange);
  m_variables.Get("thresh", thresh);
  m_variables.Get("polarity", polarity);
  m_variables.Get("TrigInEnable", TrigInEnable);
  m_variables.Get("SWTrigEnable", SWTrigEnable);
  m_variables.Get("ITLComboEnable", ITLComboEnable);
  m_variables.Get("TrigInOut", TrigInOut);
  m_variables.Get("SWTrigOut", SWTrigOut);
  m_variables.Get("AcceptedTrigOut", AcceptedTrigOut);
  m_variables.Get("DCOff", DCOff);
  m_variables.Get("bID", bID);
  m_variables.Get("verbose", verbose);
  m_variables.Get("use_ETTT", use_ETTT);
  m_variables.Get("IOLevel", IOLevel);

  ret = CAEN_FELib_SendCommand(handle, "/cmd/Reset");
  if (!ret && verbose) std::cout<<"Board reset"<<std::endl;
  else if (ret) {
    std::cout<<"Board reset failed!!!"<<std::endl;
    return false;
  }

/*
  ret = CAEN_FELib_SetUserRegister(handle,0xEF08,bID);
  if (!ret && verbose) std::cout<<"Set Board ID!"<<std::endl;
  else if (ret) {
    std::cout<<"Error setting board ID"<<std::endl;
    return false;
  }

  // Enforce record length is multiple of 20 samples (40 ns) to avoid filler words in data
  int rem = recLen%40;
  if (rem < 20) {recLen -= rem;}
  else {recLen += (40-rem);}
*/
  ret = ReadV2730::SetFloatValue(handle, "/par/RecordLengthT", recLen);
  //std::cout<<ret<<std::endl;
  ret = CAEN_FELib_GetValue(handle, "/par/RecordLengthT", value);
  if (!ret && verbose) std::cout<<"Board "<<bID<<" record Length set to: "<<value<<std::endl;
  else if (ret) {
    std::cout<<"Error setting record length on board "<<bID<<": "<<ret<<std::endl;
    return false;
  }
  ReadV2730::nsamp = std::stoi(value)/2;

  ret = ReadV2730::SetFloatValue(handle, "/par/PreTriggerT", preTrig);
  //std::cout<<ret<<std::endl;
  ret = CAEN_FELib_GetValue(handle, "/par/PreTriggerT", value);
  if (!ret && verbose) std::cout<<"Pre Trigger set to: "<<value<<" ns"<<std::endl;
  else if (ret) {
    std::cout<<"Error setting board "<<bID<<" pre trigger size: "<<ret<<std::endl;
    return false;
  }

  ret = ReadV2730::SetFloatValue(handle, "/par/TriggerDelayT", 0);
  ret = CAEN_FELib_GetValue(handle, "/par/TriggerDelayT", value);
  if (ret) {
    std::cout<<"Error setting trigger delay: "<<ret<<std::endl;
    return false;
  }
  else {
    std::cout<<"Set trigger delay to: "<<value<<std::endl;
  }

  ret = CAEN_FELib_GetValue(handle, "/par/NUMCH", value);
  int numch = std::stoi(value);
  ReadV2730::nchan = numch;

  std::string par_path = "/ch/0.." + std::to_string(numch-1) + "/par/SelfTriggerEdge";
  if (polarity!="positive" && polarity!="negative") {
    std::cout<<"Invalid trigger polarity. Must be 'positive' or 'negative'"<<std::endl;
    ret = CAEN_FELib_GenericError;
  }
  //std::string par_path = "/ch/0.." + std::to_string(numch) + "/par/SelfTriggerEdge";
  //par_path_cstr = par_path.cstr();
  else if (polarity=="positive") {
    ret = CAEN_FELib_SetValue(handle, par_path.c_str(), "RISE");
    ret = CAEN_FELib_GetValue(handle, "/ch/0/par/SelfTriggerEdge", value);
  }
  else if (polarity=="negative") {
    ret = CAEN_FELib_SetValue(handle, par_path.c_str(), "FALL");
    ret = CAEN_FELib_GetValue(handle, "/ch/0/par/SelfTriggerEdge", value);
  }
  if (!ret && verbose) std::cout<<"Board "<<bID<<" trigger polarity set to: "<<value<<std::endl;
  else if (ret) {
    std::cout<<"Error setting board "<<bID<<" trigger polarity: "<<ret<<std::endl;
    return false;
  }

  //ret = CAEN_FELib_GetValue(handle, "/par/NUMCH", value);
  //int numch = std::stoi(value);
  //ReadV2730::nchan = numch;
  for (int ch = 0; ch < numch; ch++) {
    std::string chpath = std::string("/ch/") + std::to_string(ch) + "/par/ChEnable";
    if (ChanEnableMask & (1 << ch)) {
      ret = CAEN_FELib_SetValue(handle, chpath.c_str(), "True");
    } else {
      ret = CAEN_FELib_SetValue(handle, chpath.c_str(), "False");
    }
    //std::cout<<ch<<": "<<ret<<std::endl;
    ret = CAEN_FELib_GetValue(handle, chpath.c_str(), value);
    if (ret) {
      std::cout<<"Error setting channel "<<ch<<" enable: "<<ret<<std::endl;
      return false;
    }
    //std::cout<<ch<<": "<<value<<std::endl;
    if (strcmp(value, "True")==0) mask = mask + (1<<ch);
    else if (strcmp(value, "False")!=0) std::cout<<"Weird ch enable value: "<<value<<std::endl;
  }
  if (verbose) {
    std::cout<<"Channels Enabled: "<<std::hex<<mask<<std::dec<<std::endl;
    //std::cout<<mask<<std::endl;
  }

  std::string trig_types = "";
  bool multiple = false;
  if (SWTrigEnable) {
    trig_types = "SwTrg";
    multiple = true;
  }
  if (TrigInEnable) {
    if (multiple) {
      trig_types += "|";
    }
    trig_types += "TrgIn";
    multiple = true;
  }
  if (ChanSelfTrigEnable) {
    if (multiple) trig_types += "|";
    trig_types += "ITLA";
    multiple = true;
  }
  if (ITLComboTrigEnable) {
    if (multiple) trig_types += "|";
    trig_types += "ITLA_AND_ITLB";
    multiple = true;
  }
  if (trig_types=="") {
    std::cout<<"At least one trigger type must be enabled!"<<std::endl;
    return false;
  }
  ret = CAEN_FELib_SetValue(handle, "/par/AcqTriggerSource", trig_types.c_str());
  if (ret) {
    std::cout<<"Error setting board "<<bID<<" trigger source: "<<ret<<std::endl;
    return false;
  }
  ret = CAEN_FELib_GetValue(handle, "/par/AcqTriggerSource", value);
  if (verbose) {
    std::cout<<"Board "<<bID<<" trigger source set to: "<<value<<std::endl;
  }

  std::string trig_out_types = "";
  bool multiple_out = false;
  if (SWTrigOut) {
    trig_out_types = "SwTrg";
    multiple_out = true;
  }
  if (TrigInOut) {
    if (multiple_out) {
      trig_out_types += "|";
    }
    trig_out_types += "TrgIn";
    multiple_out = true;
  }
  if (ChanSelfTrigOut) {
    if (multiple_out) trig_out_types += "|";
    trig_out_types += "ITLA";
    multiple_out = true;
  }
  if (AcceptedTrigOut) {
    if (multiple_out) trig_out_types += "|";
    trig_out_types += "AcceptTrg";
    multiple_out = true;
  }
  if (trig_out_types=="") {
    trig_out_types = "Disabled";
  }
  ret = CAEN_FELib_SetValue(handle, "/par/TrgOutMode", trig_out_types.c_str());
  if (ret) {
    std::cout<<"Error setting board "<<bID<<" TRGOUT mode: "<<ret<<std::endl;
    return false;
  }
  ret = CAEN_FELib_GetValue(handle, "/par/TrgOutMode", value);
  if (verbose) {
    std::cout<<"Board "<<bID<<" TRGOUT mode set to: "<<value<<std::endl;
  }

  ret = CAEN_FELib_SetValue(handle, "/par/StartSource", "SWcmd");
  if (ret) {
    std::cout<<"Error setting board "<<bID<<" acquisition mode: "<<ret<<std::endl;
    return false;
  }
/*
  if (use_ETTT) {
    ret = CAEN_FELib_SetValue(handle, "/par/EXTRAS_OPT", "EXTRAS_OPT_TT48");
    if (ret) {
      std::cout<<"Error setting ETTT: "<<ret<<std::endl;
      return false;
    }
  }

  ret = CAEN_DGTZ_SetMaxNumEventsBLT(handle, 10);
  if (ret) {
    std::cout<<"Error setting Max Num Events BLT"<<std::endl;
    return false;
  }

*/
  if (IOLevel=="TTL" || IOLevel=="NIM") {
    ret = CAEN_FELib_SetValue(handle, "/par/IOlevel", IOLevel.c_str());
  }
  else {
    std::cout<<"IO Level must be TTL or NIM"<<std::endl;
    return false;
  }
  ret = CAEN_FELib_GetValue(handle, "/par/IOlevel", value);
  if (!ret && verbose) {
    std::cout<<"IO Level set to: "<<value<<std::endl;
  }
  else {
    std::cout<<"Error reading board "<<bID<<" IO Level: "<<ret<<std::endl;
    return false;
  }

  std::string ITLAMainLogic, ITLBMainLogic, ITLAPairLogic, ITLBPairLogic;
  float ITLAMajorityLev, ITLBMajorityLev, ITLAGateWidth, ITLBGateWidth;

  m_variables.Get("ITLAMainLogic", ITLAMainLogic);
  m_variables.Get("ITLBMainLogic", ITLBMainLogic);
  m_variables.Get("ITLAPairLogic", ITLAPairLogic);
  m_variables.Get("ITLBPairLogic", ITLBPairLogic);
  m_variables.Get("ITLAMajorityLev", ITLAMajorityLev);
  m_variables.Get("ITLBMajorityLev", ITLBMajorityLev);
  m_variables.Get("ITLAGateWidth", ITLAGateWidth);
  m_variables.Get("ITLBGateWidth", ITLBGateWidth);

  if (ITLAMainLogic=="AND" || ITLAMainLogic=="OR" || ITLAMainLogic=="Majority") {
    ret = CAEN_FELib_SetValue(handle, "/par/ITLAMainLogic", ITLAMainLogic.c_str());
  }
  else {
    std::cout<<"ITLAMainLogic must be AND, OR, or Majority"<<std::endl;
    return false;
  }
  if (ret) {
    std::cout<<"Error setting ITLA main logic: "<<ret<<std::endl;
  }

  if (ITLAPairLogic=="OR" || ITLAPairLogic=="AND" || ITLAPairLogic=="NONE") {
    ret = CAEN_FELib_SetValue(handle, "/par/ITLAPairLogic", ITLAPairLogic.c_str());
  }
  else {
    std::cout<<"ITLAPairLogic must be OR, AND, or NONE"<<std::endl;
    return false;
  }
  if (ret) {
    std::cout<<"Error setting ITLA pair logic: "<<ret<<std::endl;
  }

  ret = ReadV2730::SetFloatValue(handle, "/par/ITLAMajorityLev", ITLAMajorityLev);
  ret = CAEN_FELib_GetValue(handle, "/par/ITLAMajorityLev", value);
  if (ret) {
    std::cout<<"Error setting board "<<bID<<" ITLA majority level: "<<ret<<std::endl;
    return false;
  }
  else if (verbose) {
    std::cout<<"Set board "<<bID<<" ITLA majority level to: "<<value<<std::endl;
  }

  ret = ReadV2730::SetFloatValue(handle, "/par/ITLAGateWidth", ITLAGateWidth);
  ret = CAEN_FELib_GetValue(handle, "/par/ITLAGateWidth", value);
  if (ret) {
    std::cout<<"Error setting board "<<bID<<" ITLA gate width: "<<ret<<std::endl;
    return false;
  }
  else if (verbose) {
    std::cout<<"Set board "<<bID<<" ITLA gate width to: "<<value<<std::endl;
  }

  if (ITLBMainLogic=="AND" || ITLBMainLogic=="OR" || ITLBMainLogic=="Majority") {
    ret = CAEN_FELib_SetValue(handle, "/par/ITLBMainLogic", ITLBMainLogic.c_str());
  }
  else {
    std::cout<<"ITLBMainLogic must be AND, OR, or Majority"<<std::endl;
    return false;
  }
  if (ret) {
    std::cout<<"Error setting ITLB main logic: "<<ret<<std::endl;
  }

  if (ITLBPairLogic=="OR" || ITLBPairLogic=="AND" || ITLBPairLogic=="NONE") {
    ret = CAEN_FELib_SetValue(handle, "/par/ITLBPairLogic", ITLBPairLogic.c_str());
  }
  else {
    std::cout<<"ITLBPairLogic must be OR, AND, or NONE"<<std::endl;
    return false;
  }
  if (ret) {
    std::cout<<"Error setting ITLB pair logic: "<<ret<<std::endl;
  }

  ret = ReadV2730::SetFloatValue(handle, "/par/ITLBMajorityLev", ITLBMajorityLev);
  ret = CAEN_FELib_GetValue(handle, "/par/ITLBMajorityLev", value);
  if (ret) {
    std::cout<<"Error setting board "<<bID<<" ITLB majority level: "<<ret<<std::endl;
    return false;
  }
  else if (verbose) {
    std::cout<<"Set board "<<bID<<" ITLB majority level to: "<<value<<std::endl;
  }

  ret = ReadV2730::SetFloatValue(handle, "/par/ITLBGateWidth", ITLBGateWidth);
  ret = CAEN_FELib_GetValue(handle, "/par/ITLBGateWidth", value);
  if (ret) {
    std::cout<<"Error setting board "<<bID<<" ITLB gate width: "<<ret<<std::endl;
    return false;
  }
  else if (verbose) {
    std::cout<<"Set board "<<bID<<" ITLB gate width to: "<<value<<std::endl;
  }

  ret = CAEN_FELib_SetValue(handle, "/par/ITLAEnRetrigger", "False");
  if (ret) {
    std::cout<<"Error setting board "<<bID<<" ITLA retrigger: "<<ret<<std::endl;
    return false;
  }

  ret = CAEN_FELib_SetValue(handle, "/par/ITLBEnRetrigger", "False");
  if (ret) {
    std::cout<<"Error setting board "<<bID<<" ITLB retrigger: "<<ret<<std::endl;
    return false;
  }

  // Set coincidence level
  //int coin_level;
  //m_variables.Get("coincidence_level", coin_level);

  // Set coincidence window
  std::string pulse_width;
  m_variables.Get("pulse_width", pulse_width);
/*
  ret = ReadV2730::SetFloatValue(handle, "/par/ITLAMajorityLev", coin_level);
  ret = CAEN_FELib_GetValue(handle, "/par/ITLAMajorityLev", value);
  if (ret) {
    std::cout<<"Error setting board "<<bID<<" majority level: "<<ret<<std::endl;
    return false;
  }
  else if (verbose) {
    std::cout<<"Set board "<<bID<<" majority level to: "<<value<<std::endl;
  }
*/
  ret = CAEN_FELib_SetValue(handle, "/ch/0..31/par/SelfTriggerWidth", pulse_width.c_str());
  if (ret) {
    std::cout<<"Error setting board "<<bID<<" pulse width: "<<ret<<std::endl;
    return false;
  }
  ret = CAEN_FELib_GetValue(handle, "/ch/0/par/SelfTriggerWidth", value);
  if (verbose) {
    std::cout<<"Board "<<bID<<" pulse width set to: "<<value<<std::endl;
  }

    int itla_mask_enable=0;
    int itlb_mask_enable=0;
    for (int ch = 0; ch < numch; ch++) {
      std::string par_ch = std::string("/ch/") + std::to_string(ch) + "/par/ITLConnect";
      if (ITLAMask & (1 << ch)) {
        ret = CAEN_FELib_SetValue(handle, par_ch.c_str(), "ITLA");
        ret = CAEN_FELib_GetValue(handle, par_ch.c_str(), value);
        //if (strcmp(value, "ITLA")==0) mask_enable = mask_enable + (1<<ch);
        if (ret) {
          std::cout<<"Error setting board "<<bID<<" channel "<<ch<<" ITLA trigger enable: "<<ret<<std::endl;
          return false;
        }
      }
      if (ITLBMask & (1 << ch)) {
        ret = CAEN_FELib_SetValue(handle, par_ch.c_str(), "ITLB");
        ret = CAEN_FELib_GetValue(handle, par_ch.c_str(), value);
        if (strcmp(value, "ITLA")==0) itla_mask_enable = itla_mask_enable + (1<<ch);
        else if (strcmp(value, "ITLB")==0) itlb_mask_enable = itlb_mask_enable + (1<<ch);
        if (ret) {
          std::cout<<"Error setting board "<<bID<<" channel "<<ch<<" ITLB trigger enable: "<<ret<<std::endl;
          return false;
        }
      }
    }
    if (verbose) {
      std::cout<<"ITLA Enable Mask: "<<std::hex<<itla_mask_enable<<std::dec<<std::endl;
      std::cout<<"ITLB Enable Mask: "<<std::hex<<itlb_mask_enable<<std::dec<<std::endl;
    }

  int use_global;
  m_variables.Get("use_global", use_global);

// Use the global settings
  if (use_global) {
    std::string par_prefix = std::string("/ch/0..") + std::to_string(numch-1) + "/par/";
    std::string par = par_prefix + "ChGain";
    int gain;
    m_variables.Get("gain", gain);
    ret = ReadV2730::SetFloatValue(handle, par.c_str(), gain);
    ret = CAEN_FELib_GetValue(handle, "/ch/0/par/ChGain", value);
    if (!ret && verbose) {
      std::cout<<"Board "<<bID<<" gain set to: "<<value<<" dB"<<std::endl;
    }
    else {
      std::cout<<"Error setting board "<<bID<<" dynamic range: "<<ret<<std::endl;
      return false;
    }
/*    ret = CAEN_FELib_GetValue(handle, "/ch/0/par/ADCToVolts", value);
    std::cout<<value<<std::endl;
    ret = CAEN_FELib_GetValue(handle, "/ch/0/par/GainFactor", value);
    std::cout<<value<<std::endl;
    ret = CAEN_FELib_GetValue(handle, "/par/InputRange", value);
    std::cout<<value<<std::endl;
*/
    par = par_prefix + "TriggerThrMode";
    ret = CAEN_FELib_SetValue(handle, par.c_str(), "Relative");
    if (ret) {
      std::cout<<"Error setting board "<<bID<<" trigger threshold mode: "<<ret<<std::endl;
      return false;
    }

    par = par_prefix + "TriggerThr";
    ret = ReadV2730::SetFloatValue(handle, par.c_str(), thresh);
    ret = CAEN_FELib_GetValue(handle, "/ch/0/par/TriggerThr", value);
    if (!ret && verbose) {
      std::cout<<"Board "<<bID<<" threshold set to: "<<value<<std::endl;
    }
    else {
      std::cout<<"Error setting board "<<bID<<" ch threshold: "<<ret<<std::endl;
      return false;
    }

    par = par_prefix + "DCOffset";
    ret = ReadV2730::SetFloatValue(handle, par.c_str(), DCOff);
    ret = CAEN_FELib_GetValue(handle, "/ch/0/par/DCOffset", value);
    if (!ret && verbose) {
      std::cout<<"Board "<<bID<<" DC Offset set to: "<<value<<std::endl;
    }
    else {
      std::cout<<"Error setting board "<<bID<<" DC Offset: "<<ret<<std::endl;
      return false;
    }
/*
    int mask_enable=0;
    for (int ch = 0; ch < numch; ch++) {
      std::string par_ch = std::string("/ch/") + std::to_string(ch) + "/par/ITLConnect";
      if (ChanSelfTrigMask & (1 << ch)) {
        ret = CAEN_FELib_SetValue(handle, par_ch.c_str(), "ITLA");
        ret = CAEN_FELib_GetValue(handle, par_ch.c_str(), value);
        if (strcmp(value, "ITLA")==0) mask_enable = mask_enable + (1<<ch);
        if (ret) {
          std::cout<<"Error setting board "<<bID<<" channel "<<ch<<" self trigger enable: "<<ret<<std::endl;
          return false;
        }
      }
    }
    if (verbose) {
      std::cout<<"Ch Self Trig Enable Mask: "<<std::hex<<mask_enable<<std::dec<<std::endl;
    }
*/
  }

// Use the individual channel settings
  else {
    std::ifstream ifile;
    std::string path;
    m_variables.Get("chan_set_file", path);

    ifile.open(path);

    if (!(ifile.is_open())) {
      std::cout<<"Error opening channel settings file"<<std::endl;
      return false;
    }

    int set_value, chan;
    std::string line, par_prefix, par_full;
    std::getline(ifile, line);

    while(std::getline(ifile, line, ',')) {

      chan = std::stoi(line);
      std::string par_prefix = std::string("/ch/") + line + "/par/";

      std::getline(ifile, line, ',');
      set_value = std::stoi(line);
      par_full = par_prefix + "DCOffset";
      ret = ReadV2730::SetFloatValue(handle, par_full.c_str(), set_value);
      if (ret) {
        std::cout<<"Error setting channel "<<chan<<" DC offset: "<<ret<<std::endl;
        return false;
      }

      std::getline(ifile, line, ',');
      set_value = std::stoi(line);
      par_full = par_prefix + "TriggerThr";
      ret = ReadV2730::SetFloatValue(handle, par_full.c_str(), set_value);
      if (ret) {
        std::cout<<"Error setting channel "<<chan<<" trigger threshold: "<<ret<<std::endl;
        return false;
      }

      if (chan%2) std::getline(ifile, line);
      else std::getline(ifile, line, ',');
      set_value = std::stoi(line);
      par_full = par_prefix + "ChGain";
      ret = ReadV2730::SetFloatValue(handle, par_full.c_str(), set_value);
      if (ret) {
        std::cout<<"Error setting channel "<<chan<<" gain: "<<ret<<std::endl;
        return false;
      }

      if (!(chan%2)) {

//        par_gen = par_prefix + "CH_TRG_GLOBAL_GEN";
//        par_prop = par_prefix + "CH_OUT_PROPAGATE";

        std::getline(ifile, line, ',');
/*        if (line=="EXTOUT_ONLY") {
          ret = CAEN_FELib_SetValue(handle, par_gen.c_str(), "FALSE");
          ret = CAEN_FELib_SetValue(handle, par_prop.c_str(), "TRUE");
        }
        else if (line=="ACQ_ONLY") {
          ret = CAEN_FELib_SetValue(handle, par_gen.c_str(), "TRUE");
          ret = CAEN_FELib_SetValue(handle, par_prop.c_str(), "FALSE");
        }
        else if (line=="ACQ_AND_EXTOUT") {
          ret = CAEN_FELib_SetValue(handle, par_gen.c_str(), "TRUE");
          ret = CAEN_FELib_SetValue(handle, par_prop.c_str(), "TRUE");
        }
        else if (line=="DISABLED") {
          ret = CAEN_FELib_SetValue(handle, par_gen.c_str(), "FALSE");
          ret = CAEN_FELib_SetValue(handle, par_prop.c_str(), "FALSE");
        }
        else {
          std::cout<<"Channel self trigger mode must be DISABLED/ACQ_ONLY/EXTOUT_ONLY/ACQ_AND_EXTOUT";
          ret = CAEN_FELib_GenericError;
        }
        if (ret) {
          std::cout<<"Error setting channel "<<chan<<" self trigger mode: "<<ret<<std::endl;
          return false;
        }
*/
        std::getline(ifile, line);
/*        par_full = par_prefix + "CH_COUPLE_TRG_MODE";
        if (line=="OR") {
          ret = CAEN_FELib_SetValue(handle, par_full.c_str(), "COUPLE_TRG_MODE_OR");
        }
        else if (line=="AND") {
          ret = CAEN_FELib_SetValue(handle, par_full.c_str(), "COUPLE_TRG_MODE_AND");
        }
        else if (line=="Xn") {
          ret = CAEN_FELib_SetValue(handle, par_full.c_str(), "COUPLE_TRG_MODE_EVEN_ONLY");
        }
        else if (line=="Xn+1") {
          ret = CAEN_FELib_SetValue(handle, par_full.c_str(), "COUPLE_TRG_MODE_ODD_ONLY");
        }
        else {
          std::cout<<"Channel self trigger logic must be OR/AND/Xn/Xn+1"<<std::endl;
          ret = CAEN_FELib_GenericError;
        }
        if (ret) {
          std::cout<<"Error setting channel "<<chan<<" self trigger logic: "<<ret<<std::endl;
          return false;
        }
*/
      }

    }
    ifile.close();
  }

  ret = CAEN_FELib_SetValue(handle, "/par/EnTriggerOverlap", "False");
  if (ret) {
    std::cout<<"Error setting board "<<bID<<" trigger overlap: "<<ret<<std::endl;
    return false;
  }

  ret = CAEN_FELib_SetValue(handle, "/par/TriggerIDMode", "EventCnt");
  if (ret) {
    std::cout<<"Error setting board "<<bID<<" trigger ID mode: "<<ret<<std::endl;
    return false;
  }

  

  return true;
}

struct event_2730* ReadV2730::allocate_event(int n_chan, int n_samps) {
  struct event_2730* evt = (struct event_2730*)malloc(sizeof(*evt));
  if (evt == NULL) {
    std::cout<<"Event malloc failed"<<std::endl;
    return NULL;
  }
  evt->waveform_size = (size_t*)malloc(n_chan * sizeof(*evt->waveform_size));
  if (evt->waveform_size == NULL) {
    std::cout<<"Waveform size malloc failed"<<std::endl;
    return NULL;
  }
  evt->waveform = (uint16_t**)malloc(n_chan * sizeof(*evt->waveform));
  if (evt->waveform == NULL) {
    std::cout<<"Waveform malloc failed"<<std::endl;
    return NULL;
  }
  for (size_t i = 0; i < n_chan; i++) {
    evt->waveform[i] = (uint16_t*)malloc(n_samps * sizeof(*evt->waveform[i]));
    if (evt->waveform[i] == NULL) {
      std::cout<<"Waveform channel malloc failed"<<std::endl;
      return NULL;
    }
  }
  return evt;
}

void ReadV2730::free_event(struct event_2730* evt, int n_chan) {
  free(evt->waveform_size);
  for (size_t i = 0; i < n_chan; i++) {
    free(evt->waveform[i]);
  }
  free(evt);
}

H5::H5File ReadV2730::OpenOutFile(std::string fname, int num_evs, int n_chan, int n_samps) {
  H5::H5File file(fname, H5F_ACC_TRUNC);

  hsize_t scalar_dims[1] = {num_evs};
  //hsize_t scalar_chunk_dims[1] = {1};
  H5::DataSpace scalar_dataspace(1, scalar_dims);
  ReadV2730::full_scalar_ds = scalar_dataspace;
  //H5::DataSpace scalar_chunk_dataspace(1, scalar_chunk_dims);

  file.createDataSet("/timestamp", H5::PredType::NATIVE_UINT64, scalar_dataspace);
  file.createDataSet("/timestamp_ns", H5::PredType::NATIVE_UINT64, scalar_dataspace);
  file.createDataSet("/trigger_id", H5::PredType::NATIVE_UINT32, scalar_dataspace);
  file.createDataSet("/flags", H5::PredType::NATIVE_UINT16, scalar_dataspace);
  //file.createDataSet("/board_id", H5::PredType::NATIVE_UINT8, scalar_dataspace);
  file.createDataSet("/board_fail", H5::PredType::NATIVE_UINT8, scalar_dataspace);

  for (int i=0; i<n_chan; i++) {
    hsize_t waveform_dims[2] = {num_evs, n_samps};
    //hsize_t waveform_chunk_dims[2] = {1, n_samps};
    H5::DataSpace waveform_dataspace(2, waveform_dims);
    ReadV2730::full_wfm_ds = waveform_dataspace;
    //H5::DataSpace waveform_chunk_dataspace(2, waveform_chunk_dims);

    std::string chname = "/ch" + std::to_string(i) + "_samples";
    file.createDataSet(chname, H5::PredType::NATIVE_UINT16, waveform_dataspace);
  }

  return file;
}

void ReadV2730::WriteEvent(H5::H5File& file, struct event_2730* evt, int event_index, int n_chan) {
  hsize_t scalar_dims[1] = {1};
  H5::DataSpace scalar_dataspace(1, scalar_dims);

  hsize_t offset[1] = {event_index};
  H5::DataSpace fileSpace = ReadV2730::full_scalar_ds;
  fileSpace.selectHyperslab(H5S_SELECT_SET, scalar_dims, offset);

  uint32_t trgIDPlusOne = evt->trigger_id + 1;

  file.openDataSet("/timestamp").write(&evt->timestamp, H5::PredType::NATIVE_UINT64, scalar_dataspace, fileSpace);
  file.openDataSet("/timestamp_ns").write(&evt->timestamp_ns, H5::PredType::NATIVE_UINT64, scalar_dataspace, fileSpace);
  file.openDataSet("/trigger_id").write(&trgIDPlusOne, H5::PredType::NATIVE_UINT32, scalar_dataspace, fileSpace);
  file.openDataSet("/flags").write(&evt->flags, H5::PredType::NATIVE_UINT16, scalar_dataspace, fileSpace);
  file.openDataSet("/board_fail").write(&evt->board_fail, H5::PredType::NATIVE_UINT8, scalar_dataspace, fileSpace);

  for (int i=0; i<n_chan; i++) {
    hsize_t waveform_dims[2] = {1, evt->waveform_size[i]};
    H5::DataSpace waveform_dataspace(2, waveform_dims);

    hsize_t waveform_offset[2] = {event_index, 0};
    H5::DataSpace wfmfileSpace = ReadV2730::full_wfm_ds;
    wfmfileSpace.selectHyperslab(H5S_SELECT_SET, waveform_dims, waveform_offset);

    std::string chname = "/ch" + std::to_string(i) + "_samples";
    file.openDataSet(chname).write(evt->waveform[i], H5::PredType::NATIVE_UINT16, waveform_dataspace, wfmfileSpace);
  }
}
