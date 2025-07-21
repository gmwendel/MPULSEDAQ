#ifndef ReadV1730Clock_H
#define ReadV1730Clock_H

#include <string>
#include <iostream>
#include <stdio.h>
#include <cstring>

#include "Tool.h"
#include "DataModel.h"
#include <CAEN_FELib.h>

/**
 * \class ReadV1730Clock
 *
 * This is a balnk template for a Tool used by the script to generate a new custom tool. Please fill out the descripton and author information.
*
* $Author: B.Richards $
* $Date: 2019/05/28 10:44:00 $
*/

class ReadV1730Clock: public Tool {


 public:

  ReadV1730Clock(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resorces. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Executre function used to perform Tool perpose. 
  bool Finalise(); ///< Finalise funciton used to clean up resorces.
  uint64_t OpenBoard(Store m_variables);

  std::string model;
  uint64_t handle;
  int bID;

 private:





};


#endif
