#ifndef SRSRAN_SCHEDULER_H
#define SRSRAN_SCHEDULER_H

#include <thread>
#include <functional>
#include <chrono>
#include <memory>
#include "predictor.h"
#include <atomic>
#include <mutex>
#include <vector>
#include "timestamp_logger.h"

// If a high-criticality job 𝐽𝑖 does not complete within its virtual deadline 𝐷′, 
// the system transitions into the critical-state. All low-criticality jobs can be abandoned 
// and future jobs of low-criticality tasks need not be admitted. 
// The scheduler now executes all jobs according to their critical-state mapping 𝑆.
// That is, all high-utilization tasks are now allocated 𝑛 dedicated cores and 
// scheduled using a work-conserving scheduler.


namespace srsran
{


class DL_scheduler{
public:
    static DL_scheduler& getInstance(){
        static DL_scheduler instance;
        return instance;
    }

    std::function<void()> check_status(unsigned nof_workers, std::function<void(unsigned)> wake_thread, std::function<void(unsigned)> sleep_thread){
        return [this, nof_workers, sleep_thread, wake_thread]() {
            while(!stop_flag.load(std::memory_order_relaxed)){
                ///std::this_thread::sleep_for(std::chrono::microseconds(20));
                if(sched_flag.load()){
                    double DL_WCET = pred->DL_predict(feature);
                    double PDSCH_WCET = pred->PDSCH_predict(feature);
                    unsigned core_num = std::max(1, int(std::ceil((DL_WCET - PDSCH_WCET) / (2 * 200 / (2 + 1.414) - PDSCH_WCET))));
                    core_num = std::min(core_num, nof_workers);
                    auto now = std::chrono::duration_cast<std::chrono::microseconds>(
                        std::chrono::system_clock::now().time_since_epoch()
                    ).count();
                    TimestampLogger::getInstance().log_timestamp_("number of waking cores", core_num, "timestamp", now);
                    if(now - sched_pt > 2 * 200 / (2 + 1.414)){
                        TimestampLogger::getInstance().log_timestamp("time limit exceed");
                        for(unsigned i = 1; i <= nof_workers; i++){
                            wake_thread(i - 1);
                        }
                    }
                    else{
                        TimestampLogger::getInstance().log_timestamp("time limit not exceed");
                        for(unsigned i = 1; i <= nof_workers; i++){
                            if(i <= core_num){
                                wake_thread(i - 1);
                            }
                            else{
                                sleep_thread(i - 1);
                            }
                        }
                    }
                }
            }
        };
    }

    void start_schedule(std::vector<double> feature_){
        std::lock_guard<std::mutex> lck(feature_mtx);
        sched_pt = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
        feature = feature_;
        sched_flag.store(true);
    }

    void DL_update(double cost){
        std::lock_guard<std::mutex> lck(feature_mtx);
        pred->DL_online_update(feature, cost);
        sched_flag.store(false);
    }

    void PDSCH_update(double cost){
        std::lock_guard<std::mutex> lck(feature_mtx);
        pred->PDSCH_online_update(feature, cost);
    }

    std::atomic<bool> stop_flag;

    std::atomic<bool> sched_flag{false};
    long sched_pt;
    std::mutex feature_mtx;
    std::vector<double> feature;

private:
    DL_scheduler(){
        pred = std::make_unique<predictor>();
        stop_flag.store(false);
    }

    ~DL_scheduler(){
        stop_flag.store(true);
    }

    std::unique_ptr<predictor> pred;
};

}
#endif