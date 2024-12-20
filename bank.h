//
// Created by 13520 on 2024/12/15.
//

#ifndef BANK_CHANDY_LAMPORT_BANK_H
#define BANK_CHANDY_LAMPORT_BANK_H
#include "bank_data.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <mutex>
#include <random>
#include <chrono>
#include <sstream>
#include <ctime>
#include <random>
#include <cstddef>
#include <algorithm>
#include <map>
#include <thread>
#define MULTICAST_GROUP "224.0.0.1"
#define MULTICAST_PORT 5007
#define BUFFER_SIZE 1024

extern int bank_number;
extern std::map<int, int> Id2Port;
class bank {
public:
    bank():balance(0),bank_id(0){
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(49152,65535);
        int tmp;
        do {
            tmp = dis(gen);
        }while(tmp == 5007 || std::any_of(Id2Port.begin(), Id2Port.end(), [tmp](const auto&pair){return pair.second == tmp;}));
        this->bank_id = bank_number;
        //this->balance = balance;
        this->bank_port = tmp;
    };


    //getter&setter
    int get_bank_id() const;
    void set_bank_id(int bankId);

    int get_balance() const;
    void set_balance(int balance);

    int get_port() const;

    //set muticast
    void setup_multicast_receiver();

    //send transfer money
    void send_transfer(int id, int amout, std::time_t timestamp);

    //receive transfer money
    void receive_transfer();

    //broadcast shapshot request
    void snapshot();

    //record self state(balance) when take snapshot
    int record();

    //display all process
    void display();


    ~bank();
private:
    int balance;
    int bank_id;
    int bank_port;
    std::string multicast_group;
    SOCKET multicast_sock;
    std::vector<std::string> inbound_messages; // 入站消息队列
    std::mutex message_mutex;
    std::atomic<bool> snapshot_flag;
};


#endif //BANK_CHANDY_LAMPORT_BANK_H
