#ifndef READV2730MT_H
#define READV2730MT_H

#include "Tool.h"
#include "ReadV2730.h"
#include <boost/lockfree/spsc_queue.hpp>
#include <thread>
#include <atomic>
#include <vector>

class ReadV2730MT: public Tool {
public:
    ReadV2730MT();
    bool Initialise(std::string configfile, DataModel &data);
    bool Execute();
    bool Finalise();

private:
    void Producer();
    void Consumer();
    void WriteBatch(std::vector<event_2730*>& batch, int start_index);

    ReadV2730 helper;
    std::thread prod_thread;
    std::thread cons_thread;
    std::atomic<bool> running{false};

    boost::lockfree::spsc_queue<event_2730*> ring;
    size_t ring_depth{1024};
    size_t batch_size{100};

    H5::H5File outfile;
    H5::DataSpace full_scalar_ds, full_wfm_ds;
    std::string ofile_part;
    int file_num{0};
    int ev_per_file{0};
    int event_index{0};
};

#endif
