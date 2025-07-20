#include "ReadV1730.h"

ReadV1730::ReadV1730():Tool(){}


bool ReadV1730::Initialise(std::string configfile, DataModel &data){

  if(configfile!="")  m_variables.Initialise(configfile);
  //m_variables.Print();

  m_data= &data;
  m_log= m_data->Log;

  if(!m_variables.Get("verbose",m_verbose)) m_verbose=1;

  uint64_t handle;
  int bID;
  int ret;

  handle = ReadV1730::OpenBoard(m_variables);
  ReadV1730::handle = handle;
  //std::cout<<"Board 1 handle: "<<handle<<std::endl;

  m_variables.Get("bID", bID);
  ReadV1730::bID = bID;

  bool test = ReadV1730::ConfigureBoard(handle, m_variables);
  if(!test) {
    std::cout<<"Board "<<bID<<" configuration failed!!!"<<std::endl;
    return true;
  }
  else {
    std::cout<<"Board "<<bID<<" configured!"<<std::endl;
  }

  ReadV1730::evt = ReadV1730::allocate_event(ReadV1730::nchan, ReadV1730::nsamp);
  if (evt == NULL) {
    std::cout<<"Error allocating event"<<std::endl;
    return false;
  }

  int ev_per_file;
  m_variables.Get("ev_per_file", ev_per_file);
  ReadV1730::ev_per_file = ev_per_file;

  std::string ofile;
  m_variables.Get("ofile", ofile);

  std::string timestamp = ReadV1730::Get_TimeStamp();
  std::string ofile_part;
  ofile_part = ofile + "_" + timestamp + "_board" + std::to_string(bID) + "_";
  ReadV1730::ofile_part = ofile_part;
  std::string ofile_full = ofile_part + "0.h5";

  ReadV1730::outfile = ReadV1730::OpenOutFile(ofile_full, ReadV1730::ev_per_file, ReadV1730::nchan, ReadV1730::nsamp);

  return true;
}


bool ReadV1730::Execute(){
/*
  uint64_t handle;
  const char *url = "dig1://caen.internal/optical_link?link_num=0&conet_node=0&vme_base_address=0";
  std::cout<<url<<std::endl;
  int ret = CAEN_FELib_Open(url, &handle);
  std::cout<<"Open V1730: "<<ret<<std::endl;
*/
  int ret, Nb=ReadV1730::Nb, Ne=ReadV1730::Ne;
  uint64_t handle=ReadV1730::handle, ep_handle, CurrentTime, ElapsedRateTime, ElapsedSWTrigTime;

  if (!ReadV1730::acq_started) {
    ret = CAEN_FELib_SetValue(handle, "/endpoint/par/activeendpoint", "scope");
    if (ret) {
      std::cout<<"Error setting active endpoint: "<<ret<<std::endl;
      return false;
    }
    ret = CAEN_FELib_GetHandle(handle, "/endpoint/scope", &ep_handle);
    if (ret) {
      std::cout<<"Error getting endpoint handle: "<<ret<<std::endl;
      return false;
    }
    ReadV1730::ep_handle = ep_handle;
    ret = CAEN_FELib_SetReadDataFormat(ep_handle, DATA_FORMAT);
    if (ret) {
      std::cout<<"Error setting read data format: "<<ret<<std::endl;
      return false;
    }

    ret = CAEN_FELib_SendCommand(handle, "/cmd/cleardata");
    if (ret) {
      std::cout<<"Error clearing data: "<<ret<<std::endl;
      return false;
    }

    ret = CAEN_FELib_SendCommand(handle, "/cmd/armacquisition");
    if (!ret) {
      std::cout<<"Acquisition started"<<std::endl;
      ReadV1730::acq_started = 1;
    }
    else {
      std::cout<<"Error starting acquisition: "<<ret<<std::endl;
      return false;
    }

    ReadV1730::Nb=0;
    ReadV1730::Ne=0;
    ReadV1730::PrevRateTime = ReadV1730::get_time();
    ReadV1730::PrevSWTrigTime = ReadV1730::get_time();
    ReadV1730::event_count=0;
  }
  else {
    CurrentTime = ReadV1730::get_time();
    ElapsedRateTime = CurrentTime - ReadV1730::PrevRateTime;
    ElapsedSWTrigTime = CurrentTime - ReadV1730::PrevSWTrigTime;

    if (ElapsedSWTrigTime > 1000) {
      ret = CAEN_FELib_SendCommand(handle, "/cmd/SendSWTrigger");
      ReadV1730::PrevSWTrigTime = ReadV1730::get_time();
    }

    ret = CAEN_FELib_ReadData(ReadV1730::ep_handle, 0,
          &ReadV1730::evt->timestamp,
          &ReadV1730::evt->timestamp_ns,
          &ReadV1730::evt->trigger_id,
          ReadV1730::evt->waveform,
          ReadV1730::evt->waveform_size,
          &ReadV1730::evt->extra,
          &ReadV1730::evt->board_id,
          &ReadV1730::evt->board_fail,
          &ReadV1730::evt->event_size
    );
    if (!ret) {
      Ne++;
      Nb += ReadV1730::evt->event_size;
      ReadV1730::event_count++;

      ReadV1730::WriteEvent(ReadV1730::outfile, ReadV1730::evt, ReadV1730::event_count - 1, ReadV1730::nchan);
    }

    if (ReadV1730::event_count == ReadV1730::ev_per_file) {
      ReadV1730::outfile.close();
      ReadV1730::file_num++;
      std::string fname = ReadV1730::ofile_part + std::to_string(ReadV1730::file_num) + ".h5";
      ReadV1730::outfile = ReadV1730::OpenOutFile(fname, ReadV1730::ev_per_file, ReadV1730::nchan, ReadV1730::nsamp);
      ReadV1730::event_count = 0;
    }

    int show_data_rate;
    m_variables.Get("show_data_rate", show_data_rate);
    if ((ElapsedRateTime > 2000) && show_data_rate) {
      if (Nb==0) std::cout<<"Board "<<ReadV1730::bID<<": No data..."<<std::endl;
      else {
        std::cout<<"Data rate Board "<<ReadV1730::bID<<": "<<(float)Nb/((float)ElapsedRateTime*1048.567f)<<" MB/s  Trigger Rate Board "<<ReadV1730::bID<<": "<<((float)Ne*1000.0f)/(float)ElapsedRateTime<<" Hz"<<std::endl;
      }

      Nb = 0;
      Ne = 0;
      ReadV1730::PrevRateTime = ReadV1730::get_time();
    }
  ReadV1730::Nb = Nb;
  ReadV1730::Ne = Ne;
  }
  return true;
}


bool ReadV1730::Finalise(){
  int ret, timeout=0;
  uint64_t handle = ReadV1730::handle;

  ret = CAEN_FELib_SendCommand(handle, "/cmd/disarmacquisition");
  if (!ret) {
    std::cout<<"Acquisition stopped"<<std::endl;
    ReadV1730::acq_started = 0;
  }
  else {
    std::cout<<"Error stopping acquisition: "<<ret<<std::endl;
    return false;
  }

  while(!timeout) {
    ret = CAEN_FELib_ReadData(ReadV1730::ep_handle, 1000,
          &ReadV1730::evt->timestamp,
          &ReadV1730::evt->timestamp_ns,
          &ReadV1730::evt->trigger_id,
          ReadV1730::evt->waveform,
          ReadV1730::evt->waveform_size,
          &ReadV1730::evt->extra,
          &ReadV1730::evt->board_id,
          &ReadV1730::evt->board_fail,
          &ReadV1730::evt->event_size
    );
    if ((ret==0) || (ret==-11) || (ret==-12)) {
      timeout = ret;
    }
    else {
      std::cout<<"Error during ReadData: "<<ret<<std::endl;
      return false;
    }
  }

  //std::cout<<"Board 1 handle2: "<<handle<<std::endl;
  ret = CAEN_FELib_Close(handle);
  std::cout<<"Close Board "<<ReadV1730::bID<<": "<<ret<<std::endl;

  ReadV1730::free_event(ReadV1730::evt, ReadV1730::nchan);

  return true;
}

//===========================================================================================

// Gets current time in milliseconds
long ReadV1730::get_time(){

  long time_ms;

  struct timeval tv;
  struct timezone tz;

  gettimeofday(&tv, &tz);

  time_ms = tv.tv_sec * 1000 + tv.tv_usec / 1000;

  return time_ms;
}

// Gets timestamp as a string (DDMMYYYYTHHMM)
std::string ReadV1730::Get_TimeStamp(){

  char timestamp[13];

  time_t rawtime;
  struct tm* timeinfo;

  time(&rawtime);
  timeinfo = localtime(&rawtime);
  strftime(timestamp, 13, "%y%m%dT%H%M", timeinfo);

  return timestamp;
}

// Opens board and returns board handle
uint64_t ReadV1730::OpenBoard(Store m_variables){

  uint32_t address;
  uint64_t handle;
  int bID, VME_bridge, LinkNum, ConetNode, verbose;
  int ret;
  char model[256], fwtype[256];

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
  ReadV1730::model = std::string(model);

  ret = CAEN_FELib_GetValue(handle, "/par/FWTYPE", fwtype);
  if (ret) std::cout<<"Error getting fw type. CAEN FELib Error Code: "<<ret<<std::endl;
  else if (!ret && verbose) {
    std::cout<<"Board "<<bID<<" FW type: "<<fwtype<<std::endl;
  }

  return handle;
}

int ReadV1730::SetFloatValue(uint64_t handle, const char *path, float val) {
  char value[256];
  snprintf(value, sizeof(value), "%f", val);
  int ret = CAEN_FELib_SetValue(handle, path, value);
  return ret;
}

// Sets board settings like trigger threshold, record length, DC offset, etc.
bool ReadV1730::ConfigureBoard(uint64_t handle, Store m_variables) {

  std::string bdname = ReadV1730::model;
  std::string tmp, tmp2, ChanSelfTrigMode, GrpSelfTrigMode;
  uint32_t ChanEnableMask, ChanSelfTrigMask, GrpEnableMask, GrpSelfTrigMask;

  if (bdname.find("730") != std::string::npos) {
    m_variables.Get("ChanEnableMask", tmp);
    m_variables.Get("ChanSelfTrigMask", tmp2);
    m_variables.Get("ChanSelfTrigMode", ChanSelfTrigMode);
    ChanEnableMask = std::stoi(tmp, 0, 16);
    ChanSelfTrigMask = std::stoi(tmp2, 0, 16);
  }

  uint32_t thresh;
  uint32_t length, percent, reg;
  uint16_t dec_factor;
  int bID, verbose, use_ETTT;
  int recLen, postTrig, mask=0;
  float dynRange, DCOff;
  char value[256];
  std::string polarity, TrigInMode, SWTrigMode, IOLevel;
  //CAEN_DGTZ_TriggerPolarity_t pol;
  //CAEN_DGTZ_TriggerMode_t selftrigmode;
  //CAEN_DGTZ_IOLevel_t iolevel;
  //CAEN_DGTZ_ErrorCode ret;
  int ret;

  m_variables.Get("RecLen", recLen);
  m_variables.Get("PostTrig", postTrig);
  m_variables.Get("DynRange", dynRange);
  m_variables.Get("thresh", thresh);
  m_variables.Get("polarity", polarity);
  m_variables.Get("TrigInMode", TrigInMode);
  m_variables.Get("SWTrigMode", SWTrigMode);
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

  if (bID==2) {
    usleep(200000); //recommended time to wait before calibrating is at least 100 ms per CAEN manual

    ret = CAEN_FELib_SendCommand(handle, "/cmd/CalibrateADC");
    if (!ret && verbose) std::cout<<"Board "<<bID<<" calibrated"<<std::endl;
    else if (ret) {
      std::cout<<"Board "<<bID<<" calibration failed!!!"<<std::endl;
      return false;
    }
  }
  
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

  ret = ReadV1730::SetFloatValue(handle, "/par/RECLEN", recLen);
  //std::cout<<ret<<std::endl;
  ret = CAEN_FELib_GetValue(handle, "/par/RECLEN", value);
  if (!ret && verbose) std::cout<<"Record Length set to: "<<value<<std::endl;
  else if (ret) {
    std::cout<<"Error setting record length: "<<ret<<std::endl;
    return false;
  }
  ReadV1730::nsamp = std::stoi(value)/2;

  ret = ReadV1730::SetFloatValue(handle, "/par/POSTTRG", postTrig);
  //std::cout<<ret<<std::endl;
  ret = CAEN_FELib_GetValue(handle, "/par/POSTTRG", value);
  if (!ret && verbose) std::cout<<"Post Trigger set to: "<<value<<std::endl;
  else if (ret) {
    std::cout<<"Error setting post trigger size: "<<ret<<std::endl;
    return false;
  }

  if (polarity!="positive" && polarity!="negative") {
    std::cout<<"Invalid trigger polarity. Must be 'positive' or 'negative'"<<std::endl;
    ret = CAEN_FELib_GenericError;
  }
  else if (polarity=="positive") {
    ret = CAEN_FELib_SetValue(handle, "/par/SELF_TRIGGER_EDGE", "RISE");
    ret = CAEN_FELib_GetValue(handle, "/par/SELF_TRIGGER_EDGE", value);
  }
  else if (polarity=="negative") {
    ret = CAEN_FELib_SetValue(handle, "/par/SELF_TRIGGER_EDGE", "FALL");
    ret = CAEN_FELib_GetValue(handle, "/par/SELF_TRIGGER_EDGE", value);
  }
  if (!ret && verbose) std::cout<<"Trigger polarity set to: "<<value<<std::endl;
  else if (ret) {
    std::cout<<"Error setting trigger polarity: "<<ret<<std::endl;
    return false;
  }

  ret = CAEN_FELib_GetValue(handle, "/par/NUMCH", value);
  int numch = std::stoi(value);
  ReadV1730::nchan = numch;
  for (int ch = 0; ch < numch; ch++) {
    std::string chpath = std::string("/ch/") + std::to_string(ch) + "/par/CH_ENABLED";
    if (ChanEnableMask & (1 << ch)) {
      ret = CAEN_FELib_SetValue(handle, chpath.c_str(), "TRUE");
    } else {
      ret = CAEN_FELib_SetValue(handle, chpath.c_str(), "FALSE");
    }
    //std::cout<<ch<<": "<<ret<<std::endl;
    ret = CAEN_FELib_GetValue(handle, chpath.c_str(), value);
    if (ret) {
      std::cout<<"Error setting channel "<<ch<<" enable: "<<ret<<std::endl;
      return false;
    }
    //std::cout<<ch<<": "<<value<<std::endl;
    if (strcmp(value, "TRUE")==0) mask = mask + (1<<ch);
    else if (strcmp(value, "FALSE")!=0) std::cout<<"Weird ch enable value: "<<value<<std::endl;
  }
  if (verbose) {
    std::cout<<"Channels Enabled: "<<std::hex<<mask<<std::dec<<std::endl;
    //std::cout<<mask<<std::endl;
  }

  if (TrigInMode=="DISABLED") {
    ret = CAEN_FELib_SetValue(handle, "/par/TRG_EXT_ENABLE", "FALSE");
    ret = CAEN_FELib_SetValue(handle, "/par/TRG_EXT_OUT_PROPAGATE", "FALSE");
  }
  else if (TrigInMode=="EXTOUT_ONLY") {
    ret = CAEN_FELib_SetValue(handle, "/par/TRG_EXT_ENABLE", "FALSE");
    ret = CAEN_FELib_SetValue(handle, "/par/TRG_EXT_OUT_PROPAGATE", "TRUE");
  }
  else if (TrigInMode=="ACQ_ONLY") {
    ret = CAEN_FELib_SetValue(handle, "/par/TRG_EXT_ENABLE", "TRUE");
    ret = CAEN_FELib_SetValue(handle, "/par/TRG_EXT_OUT_PROPAGATE", "FALSE");
  }
  else if (TrigInMode=="ACQ_AND_EXTOUT") {
    ret = CAEN_FELib_SetValue(handle, "/par/TRG_EXT_ENABLE", "TRUE");
    ret = CAEN_FELib_SetValue(handle, "/par/TRG_EXT_OUT_PROPAGATE", "TRUE");
  }
  else{
    std::cout<<"TrigInMode must be DISABLED/EXTOUT_ONLY/ACQ_ONLY/ACQ_AND_EXTOUT"<<std::endl;
    ret = CAEN_FELib_GenericError;
  }
  ret = CAEN_FELib_GetValue(handle, "/par/TRG_EXT_ENABLE", value);
  if (!ret && verbose) {
    std::cout<<"External Trigger Enable set to: "<<value<<std::endl;
  }
  else {
    std::cout<<"Error reading external trigger enable: "<<ret<<std::endl;
    return false;
  }
  ret = CAEN_FELib_GetValue(handle, "/par/TRG_EXT_OUT_PROPAGATE", value);
  if (!ret && verbose) {
    std::cout<<"External Trigger Propagation set to: "<<value<<std::endl;
  }
  else {
    std::cout<<"Error reading external trigger propagation: "<<ret<<std::endl;
    return false;
  }

  if (SWTrigMode=="DISABLED") {
    ret = CAEN_FELib_SetValue(handle, "/par/TRG_SW_ENABLE", "FALSE");
    ret = CAEN_FELib_SetValue(handle, "/par/TRG_SW_OUT_PROPAGATE", "FALSE");
  }
  else if (SWTrigMode=="EXTOUT_ONLY") {
    ret = CAEN_FELib_SetValue(handle, "/par/TRG_SW_ENABLE", "FALSE");
    ret = CAEN_FELib_SetValue(handle, "/par/TRG_SW_OUT_PROPAGATE", "TRUE");
  }
  else if (SWTrigMode=="ACQ_ONLY") {
    ret = CAEN_FELib_SetValue(handle, "/par/TRG_SW_ENABLE", "TRUE");
    ret = CAEN_FELib_SetValue(handle, "/par/TRG_SW_OUT_PROPAGATE", "FALSE");
  }
  else if (SWTrigMode=="ACQ_AND_EXTOUT") {
    ret = CAEN_FELib_SetValue(handle, "/par/TRG_SW_ENABLE", "TRUE");
    ret = CAEN_FELib_SetValue(handle, "/par/TRG_SW_OUT_PROPAGATE", "TRUE");
  }
  else{
    std::cout<<"SWTrigMode must be DISABLED/EXTOUT_ONLY/ACQ_ONLY/ACQ_AND_EXTOUT"<<std::endl;
    ret = CAEN_FELib_GenericError;
  }
  ret = CAEN_FELib_GetValue(handle, "/par/TRG_SW_ENABLE", value);
  if (!ret && verbose) {
    std::cout<<"Software Trigger Enable set to: "<<value<<std::endl;
  }
  else {
    std::cout<<"Error reading software trigger enable: "<<ret<<std::endl;
    return false;
  }
  ret = CAEN_FELib_GetValue(handle, "/par/TRG_SW_OUT_PROPAGATE", value);
  if (!ret && verbose) {
    std::cout<<"Software Trigger Propagation set to: "<<value<<std::endl;
  }
  else {
    std::cout<<"Error reading software trigger propagation: "<<ret<<std::endl;
    return false;
  }


  ret = CAEN_FELib_SetValue(handle, "/par/STARTMODE", "START_MODE_SW");
  if (ret) {
    std::cout<<"Error setting acquisition mode: "<<ret<<std::endl;
    return false;
  }

  if (use_ETTT) {
    ret = CAEN_FELib_SetValue(handle, "/par/EXTRAS_OPT", "EXTRAS_OPT_TT48");
    if (ret) {
      std::cout<<"Error setting ETTT: "<<ret<<std::endl;
      return false;
    }
  }
/*
  ret = CAEN_DGTZ_SetMaxNumEventsBLT(handle, 10);
  if (ret) {
    std::cout<<"Error setting Max Num Events BLT"<<std::endl;
    return false;
  }
*/

  if (IOLevel=="TTL" || IOLevel=="NIM") {
    IOLevel = "FPIOTYPE_"+IOLevel;
    ret = CAEN_FELib_SetValue(handle, "/par/IOLEVEL", IOLevel.c_str());
  }
  else {
    std::cout<<"IO Level must be TTL or NIM"<<std::endl;
    return false;
  }
  ret = CAEN_FELib_GetValue(handle, "/par/IOLEVEL", value);
  if (!ret && verbose) {
    std::cout<<"IO Level set to: "<<value<<std::endl;
  }
  else {
    std::cout<<"Error reading IO Level: "<<ret<<std::endl;
    return false;
  }


  // Set coincidence level
  uint32_t coin_level;
  m_variables.Get("coincidence_level", coin_level);

  // Set coincidence window
  uint32_t coin_window;
  m_variables.Get("coincidence_window", coin_window);

  uint32_t reg_data;
  ret = CAEN_FELib_GetUserRegister(handle, 0x810C, &reg_data);
  if (ret) {
    std::cout<<"Error reading register 0x810C: "<<ret<<std::endl;
    return false;
  }

  reg_data &= 0xF80FFF00;
  reg_data += ((coin_level<<24) + (coin_window<<20));

  ret = CAEN_FELib_SetUserRegister(handle, 0x810C, reg_data);
  if (ret) {
    std::cout<<"Error writing register 0x810C: "<<ret<<std::endl;
    return false;
  }

  ret = CAEN_FELib_GetUserRegister(handle, 0x810C, &reg_data);
  if (ret) {
    std::cout<<"Error reading register 0x810C: "<<ret<<std::endl;
    return false;
  }
  std::cout<<"Coincidence level set to: "<<((reg_data & 0x7000000)>>24)<<std::endl;
  std::cout<<"Coincidence window set to: "<<(((reg_data & 0xF00000)>>20)*8)<<" ns"<<std::endl;

  int use_global;
  m_variables.Get("use_global", use_global);

// Use the global settings
  if (use_global) {
    std::string par_prefix = std::string("/ch/0..") + std::to_string(numch-1) + "/par/";
    std::string par = par_prefix + "CH_INDYN";
    if (dynRange==2.0) {
      ret = CAEN_FELib_SetValue(handle, par.c_str(), "INDYN_2_0_VPP");
    }
    else if (dynRange==0.5) {
      ret = CAEN_FELib_SetValue(handle, par.c_str(), "INDYN_0_5_VPP");
    }
    else {
      std::cout<<"Dynamic Range must be 2.0 or 0.5"<<std::endl;
      return false;
    }
    ret = CAEN_FELib_GetValue(handle, "/ch/0/par/CH_INDYN", value);
    if (!ret && verbose) {
      std::cout<<"Dynamic range set to: "<<value<<std::endl;
    }
    else {
      std::cout<<"Error setting dynamic range: "<<ret<<std::endl;
      return false;
    }

    par = par_prefix + "CH_THRESHOLD";
    ret = ReadV1730::SetFloatValue(handle, par.c_str(), thresh);
    ret = CAEN_FELib_GetValue(handle, "/ch/0/par/CH_THRESHOLD", value);
    if (!ret && verbose) {
      std::cout<<"Threshold set to: "<<value<<std::endl;
    }
    else {
      std::cout<<"Error setting ch threshold: "<<ret<<std::endl;
      return false;
    }

    par = par_prefix + "CH_DCOFFSET";
    ret = ReadV1730::SetFloatValue(handle, par.c_str(), DCOff);
    ret = CAEN_FELib_GetValue(handle, "/ch/0/par/CH_DCOFFSET", value);
    if (!ret && verbose) {
      std::cout<<"DC Offset set to: "<<value<<std::endl;
    }
    else {
      std::cout<<"Error setting DC Offset: "<<ret<<std::endl;
      return false;
    }

    int mask_gen=0, mask_prop=0;
    for (int ch = 0; ch < numch; ch++) {
      std::string par_gen = std::string("/ch/") + std::to_string(ch) + "/par/CH_TRG_GLOBAL_GEN";
      std::string par_prop = std::string("/ch/") + std::to_string(ch) + "/par/CH_OUT_PROPAGATE";
      if (ChanSelfTrigMask & (1 << ch)) {
        if (ChanSelfTrigMode=="DISABLED") {
          ret = CAEN_FELib_SetValue(handle, par_gen.c_str(), "FALSE");
          ret = CAEN_FELib_SetValue(handle, par_prop.c_str(), "FALSE");
        }
        else if (ChanSelfTrigMode=="EXTOUT_ONLY") {
          ret = CAEN_FELib_SetValue(handle, par_gen.c_str(), "FALSE");
          ret = CAEN_FELib_SetValue(handle, par_prop.c_str(), "TRUE");
        }
        else if (ChanSelfTrigMode=="ACQ_ONLY") {
          ret = CAEN_FELib_SetValue(handle, par_gen.c_str(), "TRUE");
          ret = CAEN_FELib_SetValue(handle, par_prop.c_str(), "FALSE");
        }
        else if (ChanSelfTrigMode=="ACQ_AND_EXTOUT") {
          ret = CAEN_FELib_SetValue(handle, par_gen.c_str(), "TRUE");
          ret = CAEN_FELib_SetValue(handle, par_prop.c_str(), "TRUE");
        }
        else {
          std::cout<<"ChanSelfTrigMode must be DISABLED/EXTOUT_ONLY/ACQ_ONLY/ACQ_AND_EXTOUT"<<std::endl;
          return false;
        }
        ret = CAEN_FELib_GetValue(handle, par_gen.c_str(), value);
        if (strcmp(value, "TRUE")==0) mask_gen = mask_gen + (1<<ch);
        ret = CAEN_FELib_GetValue(handle, par_prop.c_str(), value);
        if (strcmp(value, "TRUE")==0) mask_prop = mask_prop + (1<<ch);
        if (ret) {
          std::cout<<"Error setting channel "<<ch<<" self trigger mode: "<<ret<<std::endl;
          return false;
        }
      }
    }
    if (verbose) {
      std::cout<<"Ch Self Trig Acq Mask: "<<std::hex<<mask_gen<<std::dec<<std::endl;
      std::cout<<"Ch Self Trig ExtOut Mask: "<<std::hex<<mask_prop<<std::dec<<std::endl;
    }
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
    std::string line, par_prefix, par_full, par_gen, par_prop;
    std::getline(ifile, line);

    while(std::getline(ifile, line, ',')) {

      chan = std::stoi(line);
      std::string par_prefix = std::string("/ch/") + line + "/par/";

      std::getline(ifile, line, ',');
      set_value = std::stoi(line);
      par_full = par_prefix + "CH_DCOFFSET";
      ret = ReadV1730::SetFloatValue(handle, par_full.c_str(), set_value);
      if (ret) {
        std::cout<<"Error setting channel "<<chan<<" DC offset: "<<ret<<std::endl;
        return false;
      }

      std::getline(ifile, line, ',');
      set_value = std::stoi(line);
      par_full = par_prefix + "CH_THRESHOLD";
      ret = ReadV1730::SetFloatValue(handle, par_full.c_str(), set_value);
      if (ret) {
        std::cout<<"Error setting channel "<<chan<<" trigger threshold: "<<ret<<std::endl;
        return false;
      }

      if (chan%2) std::getline(ifile, line);
      else std::getline(ifile, line, ',');
      par_full = par_prefix + "CH_INDYN";
      if (line=="2.0") {
        ret = CAEN_FELib_SetValue(handle, par_full.c_str(), "INDYN_2_0_VPP");
      }
      else if (line=="0.5") {
        ret = CAEN_FELib_SetValue(handle, par_full.c_str(), "INDYN_0_5_VPP");
      }
      else {
        ret = CAEN_FELib_GenericError;
        std::cout<<"Invalid dynamic range, must be 2.0 or 0.5"<<std::endl;
      }
      if (ret) {
        std::cout<<"Error setting channel "<<chan<<" dynamic range: "<<ret<<std::endl;
        return false;
      }

      if (!(chan%2)) {

        par_gen = par_prefix + "CH_TRG_GLOBAL_GEN";
        par_prop = par_prefix + "CH_OUT_PROPAGATE";

        std::getline(ifile, line, ',');
        if (line=="EXTOUT_ONLY") {
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

        std::getline(ifile, line);
        par_full = par_prefix + "CH_COUPLE_TRG_MODE";
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
      }

    }
    ifile.close();
  }

  return true;
}

struct event* ReadV1730::allocate_event(int n_chan, int n_samps) {
  struct event* evt = (struct event*)malloc(sizeof(*evt));
  if (evt == NULL) {
    std::cout<<"Event malloc failed"<<std::endl;
    return NULL;
  }
  evt->waveform_size = (size_t*)malloc(n_chan * sizeof(*evt->waveform_size));
  if (evt->waveform_size == NULL) {
    std::cout<<"Waveform size malloc failed"<<std::endl;
    return NULL;
  }
  evt->waveform = (int16_t**)malloc(n_chan * sizeof(*evt->waveform));
  if (evt->waveform == NULL) {
    std::cout<<"Waveform malloc failed"<<std::endl;
    return NULL;
  }
  for (size_t i = 0; i < n_chan; i++) {
    evt->waveform[i] = (int16_t*)malloc(n_samps * sizeof(*evt->waveform[i]));
    if (evt->waveform[i] == NULL) {
      std::cout<<"Waveform channel malloc failed"<<std::endl;
      return NULL;
    }
  }
  return evt;
}

void ReadV1730::free_event(struct event* evt, int n_chan) {
  free(evt->waveform_size);
  for (size_t i = 0; i < n_chan; i++) {
    free(evt->waveform[i]);
  }
  free(evt);
}

H5::H5File ReadV1730::OpenOutFile(std::string fname, int num_evs, int n_chan, int n_samps) {
  H5::H5File file(fname, H5F_ACC_TRUNC);

  hsize_t scalar_dims[1] = {num_evs};
  //hsize_t scalar_chunk_dims[1] = {1};
  H5::DataSpace scalar_dataspace(1, scalar_dims);
  ReadV1730::full_scalar_ds = scalar_dataspace;
  //H5::DataSpace scalar_chunk_dataspace(1, scalar_chunk_dims);

  file.createDataSet("/timestamp", H5::PredType::NATIVE_UINT64, scalar_dataspace);
  file.createDataSet("/timestamp_ns", H5::PredType::NATIVE_UINT64, scalar_dataspace);
  file.createDataSet("/trigger_id", H5::PredType::NATIVE_UINT32, scalar_dataspace);
  file.createDataSet("/extra", H5::PredType::NATIVE_UINT16, scalar_dataspace);
  //file.createDataSet("/board_id", H5::PredType::NATIVE_UINT8, scalar_dataspace);
  file.createDataSet("/board_fail", H5::PredType::NATIVE_UINT8, scalar_dataspace);

  for (int i=0; i<n_chan; i++) {
    hsize_t waveform_dims[2] = {num_evs, n_samps};
    //hsize_t waveform_chunk_dims[2] = {1, n_samps};
    H5::DataSpace waveform_dataspace(2, waveform_dims);
    ReadV1730::full_wfm_ds = waveform_dataspace;
    //H5::DataSpace waveform_chunk_dataspace(2, waveform_chunk_dims);

    std::string chname = "/ch" + std::to_string(i) + "_samples";
    file.createDataSet(chname, H5::PredType::NATIVE_INT16, waveform_dataspace);
  }

  return file;
}

void ReadV1730::WriteEvent(H5::H5File& file, struct event* evt, int event_index, int n_chan) {
  hsize_t scalar_dims[1] = {1};
  H5::DataSpace scalar_dataspace(1, scalar_dims);

  hsize_t offset[1] = {event_index};
  H5::DataSpace fileSpace = ReadV1730::full_scalar_ds;
  fileSpace.selectHyperslab(H5S_SELECT_SET, scalar_dims, offset);

  file.openDataSet("/timestamp").write(&evt->timestamp, H5::PredType::NATIVE_UINT64, scalar_dataspace, fileSpace);
  file.openDataSet("/timestamp_ns").write(&evt->timestamp_ns, H5::PredType::NATIVE_UINT64, scalar_dataspace, fileSpace);
  file.openDataSet("/trigger_id").write(&evt->trigger_id, H5::PredType::NATIVE_UINT32, scalar_dataspace, fileSpace);
  file.openDataSet("/extra").write(&evt->extra, H5::PredType::NATIVE_UINT16, scalar_dataspace, fileSpace);
  file.openDataSet("/board_fail").write(&evt->board_fail, H5::PredType::NATIVE_UINT8, scalar_dataspace, fileSpace);

  for (int i=0; i<n_chan; i++) {
    hsize_t waveform_dims[2] = {1, evt->waveform_size[i]};
    H5::DataSpace waveform_dataspace(2, waveform_dims);

    hsize_t waveform_offset[2] = {event_index, 0};
    H5::DataSpace wfmfileSpace = ReadV1730::full_wfm_ds;
    wfmfileSpace.selectHyperslab(H5S_SELECT_SET, waveform_dims, waveform_offset);

    std::string chname = "/ch" + std::to_string(i) + "_samples";
    file.openDataSet(chname).write(evt->waveform[i], H5::PredType::NATIVE_INT16, waveform_dataspace, wfmfileSpace);
  }
}
