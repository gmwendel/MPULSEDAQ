#ifndef READV1730MT_H
#define READV1730MT_H

#include "Tool.h"
#include "ReadV1730.h"
#include <boost/lockfree/spsc_queue.hpp>
#include <thread>
#include <atomic>
#include <vector>

class ReadV1730MT: public Tool {
public:
    ReadV1730MT();
    bool Initialise(std::string configfile, DataModel &data);
    bool Execute();
    bool Finalise();

private:
    void Producer();
    void Consumer();
    void WriteBatch(std::vector<event*>& batch, int start_index);

    ReadV1730 helper;
    std::thread prod_thread;
    std::thread cons_thread;
    std::atomic<bool> running{false};

    boost::lockfree::spsc_queue<event*> ring;
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
