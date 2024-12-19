//
// Created by 13520 on 2024/12/9.
//

#ifndef DISTRIBUTE_OS_HW_BANK_DATA_H
#define DISTRIBUTE_OS_HW_BANK_DATA_H

#include <string>
#include <chrono>
#include <sstream>
class bank_data {
public:
    bank_data() : transfer(0), time(std::chrono::system_clock::now()) {}
    bank_data(int transfer) : time(std::chrono::system_clock::now()) {
        this->transfer = transfer;
    }
    int get_transfer();
    void set_transfer(int transfer);

    std::chrono::system_clock::time_point get_time();
    void set_time(std::chrono::system_clock::time_point time);

    std::string serialize() const;
    static bank_data deserialize(const std::string& data);

private:
    int transfer;
    std::chrono::system_clock::time_point time;
};


#endif //DISTRIBUTE_OS_HW_BANK_DATA_H
