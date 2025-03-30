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
#include <fstream>
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
            std::ofstream csv_file("task log.csv", std::ios::out);
            csv_file << "timestamp,core_num,num1,num2,start_point\n";
            while(!stop_flag.load(std::memory_order_relaxed)){
                std::this_thread::sleep_for(std::chrono::microseconds(20));
                auto now = std::chrono::duration_cast<std::chrono::microseconds>(
                        std::chrono::system_clock::now().time_since_epoch()
                ).count();
                if(sched_flag.load()){
                    double DL_WCET = pred->DL_predict(feature);
                    double PDSCH_WCET = pred->PDSCH_predict(feature);
                    unsigned core_num = std::max(std::max(int(std::ceil(DL_WCET / 500)), 1), int(std::ceil((DL_WCET - PDSCH_WCET) / (292.9 - PDSCH_WCET))));
                    core_num = std::min(core_num, nof_workers);
                    csv_file << now << "," << core_num << "," << DL_WCET << "," << 
                    std::ceil((DL_WCET - PDSCH_WCET) / (292.9 - PDSCH_WCET)) << "," <<
                    sched_pt << std::endl;
                    sched_flag.store(false);
                    if(core_num < last_core_num){
                        for(unsigned i = core_num + 1; i <= last_core_num; i++){
                            sleep_thread(i - 1);
                        }
                    }
                    else if(core_num > last_core_num){
                        for(unsigned i = last_core_num + 1; i <= core_num; i++){
                            wake_thread(i - 1);
                        }
                    }
                    else{
                        continue;
                    }
                    last_core_num = core_num;
                }
                else{
                    if(now - sched_pt > 292.9 && !is_finished.load() && last_core_num < nof_workers){
                        for(unsigned i = last_core_num + 1; i <= nof_workers; i++){
                            wake_thread(i - 1);
                        }
                        last_core_num = nof_workers;
                        csv_file << now << "," << last_core_num << "," << -100 << "," << 
                        -100 << "," << sched_pt << std::endl;
                    }
                }
            }
        };
    }

    void start_schedule(std::vector<double> feature_){
        sched_pt = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
        feature = feature_;
        sched_flag.store(true);
        is_finished.store(false);
    }

    void DL_update(double cost){
        pred->DL_online_update(feature, cost);
        is_finished.store(true);
    }

    void PDSCH_update(double cost){
        pred->PDSCH_online_update(feature, cost);
    }

    std::atomic<bool> stop_flag;
    std::atomic<bool> sched_flag{false};
    std::atomic<bool> is_finished{false};
    long sched_pt;
    unsigned last_core_num = 8;
    std::mutex feature_mtx;
    std::vector<double> feature;

private:
    DL_scheduler(){
        pred = std::make_unique<predictor>();
        feature = {0, 0, 0, 0, 0, 0, 0, 0};
        stop_flag.store(false);
    }

    ~DL_scheduler(){
        stop_flag.store(true);
    }

    std::unique_ptr<predictor> pred;
};

}
#endif