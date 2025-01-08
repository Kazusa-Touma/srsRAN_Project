#pragma once

#include <fstream>
#include <mutex>
#include <string>
#include <chrono>
#include "../../../include/srsran/ran/slot_point.h"

class TimestampLogger {
public:
    static TimestampLogger& getInstance() {
        static TimestampLogger instance;
        return instance;
    }

    void log_timestamp_(const std::string& event_name, double pred) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!file_.is_open()) {
            file_.open("timestamps.csv", std::ios::out);
            if (!file_.is_open()) {
                return;
            }
            // 写入CSV头
            if (file_.tellp() == 0) {
                file_ << "Event,Timestamp(us)" << std::endl;
            }
        }
        file_ << event_name << ":" << pred << std::endl;
    }

    void log_timestamp(srsran::slot_point sl_tx) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!file_.is_open()) {
            file_.open("timestamps.csv", std::ios::out);
            if (!file_.is_open()) {
                return;
            }
            // 写入CSV头
            if (file_.tellp() == 0) {
                file_ << "Event,Timestamp(us)" << std::endl;
            }
        }
        file_ << fmt::format("{}", sl_tx)<< std::endl;
    }

    void log_timestamp(double num) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!file_.is_open()) {
            file_.open("timestamps.csv", std::ios::out);
            if (!file_.is_open()) {
                return;
            }
            // 写入CSV头
            if (file_.tellp() == 0) {
                file_ << "Event,Timestamp(us)" << std::endl;
            }
        }
        file_ << num << std::endl;
    }

    void log_timestamp(std::string num) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!file_.is_open()) {
            file_.open("timestamps.csv", std::ios::out);
            if (!file_.is_open()) {
                return;
            }
            // 写入CSV头
            if (file_.tellp() == 0) {
                file_ << "Event,Timestamp(us)" << std::endl;
            }
        }
        file_ << num << std::endl;
    }
    
    void log_timestamp(const std::string& event_name, uint64_t timestamp) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!file_.is_open()) {
            file_.open("timestamps.csv", std::ios::out);
            if (!file_.is_open()) {
                return;
            }
            // 写入CSV头
            if (file_.tellp() == 0) {
                file_ << "Event,Timestamp(us)" << std::endl;
            }
        }
        file_ << event_name << ":" << timestamp;
        if(event_name == "MAC_to_PHY"){
            file_ << std::endl;
        }
        else{
            file_ << ", ";
        }
    }

    void log_timestamp(const std::string& event_name1, uint64_t timestamp1, const std::string& event_name2, uint64_t timestamp2){
        std::lock_guard<std::mutex> lock(mutex_);
        if (!file_.is_open()) {
            file_.open("timestamps.csv", std::ios::out);
            if (!file_.is_open()) {
                return;
            }
            // 写入CSV头
            if (file_.tellp() == 0) {
                file_ << "Event,Timestamp(us)" << std::endl;
            }
        }
        file_ << event_name1 << ":" << timestamp1 << ", " << event_name2 << ":" << timestamp2 << ", " << "Duration" << ":" << timestamp2 - timestamp1 << std::endl;
    }

    void log_timestamp(const std::string& event_name1, uint64_t timestamp1, const std::string& event_name2, uint64_t timestamp2, srsran::slot_point sl_tx){
        std::lock_guard<std::mutex> lock(mutex_);
        if (!file_.is_open()) {
            file_.open("timestamps.csv", std::ios::out);
            if (!file_.is_open()) {
                return;
            }
            // 写入CSV头
            if (file_.tellp() == 0) {
                file_ << "Event,Timestamp(us)" << std::endl;
            }
        }
        file_ << event_name1 << ":" << timestamp1 << ", " << event_name2 << ":" << timestamp2 << ", " << "Duration" << ":" << timestamp2 - timestamp1 << " " << fmt::format("{}", sl_tx) << std::endl;
    }

    void log_timestamp(const std::string& event_name, uint64_t timestamp, uint64_t size){
        std::lock_guard<std::mutex> lock(mutex_);
        if (!file_.is_open()) {
            file_.open("timestamps.csv", std::ios::out);
            if (!file_.is_open()) {
                return;
            }
            // 写入CSV头
            if (file_.tellp() == 0) {
                file_ << "Event,Timestamp(us)" << std::endl;
            }
        }
        file_ << event_name << ":" << timestamp << "," << "TBS size:" << size << std::endl;
    }

    void log_timestamp(const std::string& event_name, uint64_t timestamp, uint64_t size, srsran::slot_point sl_tx){
        std::lock_guard<std::mutex> lock(mutex_);
        if (!file_.is_open()) {
            file_.open("timestamps.csv", std::ios::out);
            if (!file_.is_open()) {
                return;
            }
            // 写入CSV头
            if (file_.tellp() == 0) {
                file_ << "Event,Timestamp(us)" << std::endl;
            }
        }
        file_ << event_name << ":" << timestamp << "," << "TBS size:" << size << " " << fmt::format("{}", sl_tx) << std::endl;
    }

    void log_timestamp(const std::string& event_name, double dl_bs, double DL, uint64_t timestamp){
        std::lock_guard<std::mutex> lock(mutex_);
        if (!file_.is_open()) {
            file_.open("timestamps.csv", std::ios::out);
            if (!file_.is_open()) {
                return;
            }
            // 写入CSV头
            if (file_.tellp() == 0) {
                file_ << "Event,Timestamp(us)" << std::endl;
            }
        }
        file_ << "bs update time:" << timestamp << "," << event_name << ":" << dl_bs << "," << "DL brate:" << DL << std::endl;
    }

    void log_timestamp(const std::string& event_name1, unsigned value1, const std::string& event_name2, unsigned value2, const std::string& event_name3, unsigned value3){
        std::lock_guard<std::mutex> lock(mutex_);
        if (!file_.is_open()) {
            file_.open("timestamps.csv", std::ios::out);
            if (!file_.is_open()) {
                return;
            }
            // 写入CSV头
            if (file_.tellp() == 0) {
                file_ << "Event,Timestamp(us)" << std::endl;
            }
        }
        file_ << event_name1 << ":" << value1 << ", " << event_name2 << ":" << value2 << ", " << event_name3 << ":" << value3 << std::endl;
    }

private:
    TimestampLogger() {}
    ~TimestampLogger() {
        if (file_.is_open()) {
            file_.close();
        }
    }

    std::ofstream file_;
    std::mutex mutex_;
}; 