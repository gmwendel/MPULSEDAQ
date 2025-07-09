#include "ReadV1730MT.h"
#include <chrono>

ReadV1730MT::ReadV1730MT(): Tool(), ring(1024) {}

bool ReadV1730MT::Initialise(std::string configfile, DataModel &data){
    if(configfile!="") m_variables.Initialise(configfile);
    m_data=&data;
    m_log=m_data->Log;

    if(!m_variables.Get("verbose",m_verbose)) m_verbose=1;
    if(!m_variables.Get("batch_size", batch_size)) batch_size=100;
    if(!m_variables.Get("ring_depth", ring_depth)) ring_depth=1024;

    ring.set_capacity(ring_depth);

    uint64_t handle = helper.OpenBoard(m_variables);
    helper.handle = handle;

    int bID; m_variables.Get("bID", bID); helper.bID=bID;
    if(!helper.ConfigureBoard(handle, m_variables)){
        std::cout<<"Board "<<bID<<" configuration failed!!!"<<std::endl;
        return true;
    }

    helper.evt = helper.allocate_event(helper.nchan, helper.nsamp);

    m_variables.Get("ev_per_file", ev_per_file);
    std::string ofile; m_variables.Get("ofile", ofile);
    ofile_part = ofile + "_" + helper.Get_TimeStamp() + "_board" + std::to_string(bID) + "_";
    std::string ofile_full = ofile_part + "0.h5";

    outfile = helper.OpenOutFile(ofile_full, ev_per_file, helper.nchan, helper.nsamp);
    full_scalar_ds = helper.full_scalar_ds;
    full_wfm_ds = helper.full_wfm_ds;

    running=true;
    prod_thread = std::thread(&ReadV1730MT::Producer, this);
    cons_thread = std::thread(&ReadV1730MT::Consumer, this);

    return true;
}

bool ReadV1730MT::Execute(){
    return true;
}

bool ReadV1730MT::Finalise(){
    running=false;
    if(prod_thread.joinable()) prod_thread.join();
    if(cons_thread.joinable()) cons_thread.join();
    outfile.close();
    CAEN_FELib_Close(helper.handle);
    return true;
}

void ReadV1730MT::Producer(){
    while(running){
        event* evt = helper.allocate_event(helper.nchan, helper.nsamp);
        int ret = CAEN_FELib_ReadData(helper.ep_handle, 0,
                &evt->timestamp,
                &evt->timestamp_ns,
                &evt->trigger_id,
                evt->waveform,
                evt->waveform_size,
                &evt->extra,
                &evt->board_id,
                &evt->board_fail,
                &evt->event_size);
        if(!ret){
            while(!ring.push(evt) && running){
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
        } else {
            helper.free_event(evt, helper.nchan);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}

void ReadV1730MT::WriteBatch(std::vector<event*>& batch, int start_index){
    size_t n = batch.size();
    if(n==0) return;
    hsize_t count[1] = {n};
    hsize_t offset[1] = {static_cast<hsize_t>(start_index)};
    H5::DataSpace memScalar(1, count);
    H5::DataSpace fileScalar = full_scalar_ds;
    fileScalar.selectHyperslab(H5S_SELECT_SET, count, offset);

    std::vector<uint64_t> ts(n), tns(n);
    std::vector<uint32_t> trig(n);
    std::vector<uint16_t> extra(n);
    std::vector<uint8_t> fail(n);
    for(size_t i=0;i<n;++i){
        ts[i]=batch[i]->timestamp;
        tns[i]=batch[i]->timestamp_ns;
        trig[i]=batch[i]->trigger_id;
        extra[i]=batch[i]->extra;
        fail[i]=batch[i]->board_fail;
    }
    outfile.openDataSet("/timestamp").write(ts.data(), H5::PredType::NATIVE_UINT64, memScalar, fileScalar);
    outfile.openDataSet("/timestamp_ns").write(tns.data(), H5::PredType::NATIVE_UINT64, memScalar, fileScalar);
    outfile.openDataSet("/trigger_id").write(trig.data(), H5::PredType::NATIVE_UINT32, memScalar, fileScalar);
    outfile.openDataSet("/extra").write(extra.data(), H5::PredType::NATIVE_UINT16, memScalar, fileScalar);
    outfile.openDataSet("/board_fail").write(fail.data(), H5::PredType::NATIVE_UINT8, memScalar, fileScalar);

    size_t nsamp = batch[0]->waveform_size[0];
    for(int ch=0; ch<helper.nchan; ++ch){
        hsize_t dims[2] = {n, nsamp};
        hsize_t offset2[2] = {static_cast<hsize_t>(start_index),0};
        H5::DataSpace memWfm(2,dims);
        H5::DataSpace fileWfm = full_wfm_ds;
        fileWfm.selectHyperslab(H5S_SELECT_SET, dims, offset2);
        std::vector<int16_t> buf(n*nsamp);
        for(size_t i=0;i<n;++i){
            memcpy(&buf[i*nsamp], batch[i]->waveform[ch], nsamp*sizeof(int16_t));
        }
        std::string chname = "/ch"+std::to_string(ch)+"_samples";
        outfile.openDataSet(chname).write(buf.data(), H5::PredType::NATIVE_INT16, memWfm, fileWfm);
    }
}

void ReadV1730MT::Consumer(){
    std::vector<event*> batch;
    batch.reserve(batch_size);
    while(running || !ring.empty()){
        event* evt = nullptr;
        if(ring.pop(evt)){
            batch.push_back(evt);
            if(batch.size()>=batch_size){
                WriteBatch(batch, event_index);
                event_index += batch.size();
                for(event* e : batch) helper.free_event(e, helper.nchan);
                batch.clear();
                if(event_index>=ev_per_file){
                    outfile.close();
                    file_num++;
                    std::string fname = ofile_part + std::to_string(file_num) + ".h5";
                    outfile = helper.OpenOutFile(fname, ev_per_file, helper.nchan, helper.nsamp);
                    full_scalar_ds = helper.full_scalar_ds;
                    full_wfm_ds = helper.full_wfm_ds;
                    event_index=0;
                }
            }
        }else{
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
    if(!batch.empty()){
        WriteBatch(batch, event_index);
        for(event* e : batch) helper.free_event(e, helper.nchan);
        batch.clear();
    }
}

