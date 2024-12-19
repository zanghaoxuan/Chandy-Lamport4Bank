//
// Created by 13520 on 2024/12/9.
//

#include "bank_data.h"

int bank_data::get_transfer() {
    return transfer;
}

void bank_data::set_transfer(int transfer) {
    bank_data::transfer = transfer;
}

std::chrono::system_clock::time_point bank_data::get_time(){
    return time;
}

void bank_data::set_time(std::chrono::system_clock::time_point time){
    bank_data ::time = time;
}

std::string bank_data::serialize() const {
    auto tmp = std::chrono::system_clock::to_time_t(time);
    std::stringstream ss;
    ss << transfer << "," << tmp << ";";
    return ss.str();
}

bank_data bank_data::deserialize(const std::string &data) {
    std::stringstream  ss(data);
    std::string token;
    int transfer_value;
    std::time_t time_value;

    std::getline(ss, token, ',');
    transfer_value = std::stof(token);
    std::getline(ss, token, ';');
    time_value = std::stoll(token);

    bank_data obj(transfer_value);
    obj.set_time(std::chrono::system_clock::from_time_t(time_value));
    return obj;
}