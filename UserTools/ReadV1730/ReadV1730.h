#ifndef ReadV1730_H
#define ReadV1730_H

#include <string>
#include <iostream>
#include <stdio.h>
#include <cstring>
#include <sys/time.h>
#include <ctime>

#include "Tool.h"
#include "DataModel.h"
#include <CAEN_FELib.h>
#include <H5Cpp.h>

#define DATA_FORMAT " \
        [ \
                { \"name\" : \"TIMESTAMP\", \"type\" : \"U64\" }, \
                { \"name\" : \"TIMESTAMP_NS\", \"type\" : \"U64\" }, \
                { \"name\" : \"TRIGGER_ID\", \"type\" : \"U32\" }, \
                { \"name\" : \"WAVEFORM\", \"type\" : \"I16\", \"dim\" : 2 }, \
                { \"name\" : \"WAVEFORM_SIZE\", \"type\" : \"SIZE_T\", \"dim\" : 1 }, \
                { \"name\" : \"EXTRA\", \"type\" : \"U16\" }, \
                { \"name\" : \"BOARD_ID\", \"type\" : \"U8\" }, \
                { \"name\" : \"BOARD_FAIL\", \"type\" : \"BOOL\" }, \
                { \"name\" : \"EVENT_SIZE\", \"type\" : \"U32\" } \
        ] \
"

/**
 * \class ReadV1730
 *
 * This is a balnk template for a Tool used by the script to generate a new custom tool. Please fill out the descripton and author information.
*
* $Author: B.Richards $
* $Date: 2019/05/28 10:44:00 $
*/

class ReadV1730: public Tool {



 public:

  ReadV1730(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resorces. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Executre function used to perform Tool perpose. 
  bool Finalise(); ///< Finalise funciton used to clean up resorces.

  long get_time();
  std::string Get_TimeStamp();
  uint64_t OpenBoard(Store m_variables);
  int SetFloatValue(uint64_t handle, const char *path, float val);
  bool ConfigureBoard(uint64_t handle, Store m_variables);
  struct event* allocate_event(int n_chan, int n_samps);
  void free_event(struct event* evt, int n_chan);

  H5::H5File OpenOutFile(std::string fname, int num_evs, int n_chan, int n_samps);
  void WriteEvent(H5::H5File& file, struct event* evt, int event_index, int n_chan);

  std::string model, ofile_part;
  uint64_t handle, ep_handle, PrevRateTime, PrevTempTime, PrevSWTrigTime;
  int bID, nsamp, nchan, ev_per_file=0, file_num=0, store_temps, acq_started=0, event_count, Nb, Ne;
  std::ofstream tfile;
  H5::H5File outfile;
  H5::DataSpace full_scalar_ds, full_wfm_ds;

  struct event* evt;

 private:





};

struct event {
        uint64_t timestamp;
        uint64_t timestamp_ns;
        uint32_t trigger_id;
        int16_t** waveform;
        size_t* waveform_size;
        uint16_t extra;
        uint8_t board_id;
        bool board_fail;
        uint32_t event_size;
};

#endif
