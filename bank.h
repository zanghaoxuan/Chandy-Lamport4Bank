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
#include <QObject>
#include <QDebug>
#include <QTime>
#define MULTICAST_GROUP "224.0.0.1"
#define MULTICAST_PORT 5007
#define BUFFER_SIZE 1024

extern int bank_number;
extern std::map<int, int> Id2Port;
extern std::map<int, std::map<int, bool>> channelSnapshotCompleted;             // ���ÿ��ͨ�����ھӣ��Ŀ���״̬
extern std::map<int, time_t> SnapshotTime;//��¼ÿ���ڵ������յ�ʱ�䣬��һ���յ�������Ϣ��ʱ��
extern std::map<int, std::map<int, int>> ChannelState;//�ŵ��ϵ���Ϣ״̬�����շ�->������->���
extern time_t start_snapshot_time;


class bank : public QObject{
    Q_OBJECT
public:
    std::map<int, std::map<int, std::map<time_t, time_t>>> inboundChannels;  // �洢ÿ���ھӣ�ͨ�������յ�����Ϣ          ���ͷ�, <<����ʱ�䣬����ʱ��>, ���>

    bank():balance(0),bank_id(0){
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(49152,65535);
        int tmp;
        do {
            tmp = dis(gen);
        }while(tmp == 5007 || std::any_of(Id2Port.begin(), Id2Port.end(), [tmp](const auto&pair){return pair.second == tmp;}));
        this->bank_id = bank_number;
        this->first_time_snapshot = false;
        this->bank_port = tmp;

        // ��ʼ�� WinSock
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);

        memset(&addr, 0, sizeof(addr));
        this->addr.sin_family = AF_INET;
        this->addr.sin_port = htons(this->bank_port); // �̶����ͷ��˿�
        this->addr.sin_addr.s_addr = INADDR_ANY;

        this->sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

        // ���ö˿ڸ��ã������ʧ��
        int opt = 1;
        setsockopt(this->sock, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

        // �󶨶˿�
        if (bind(this->sock, (sockaddr*)&this->addr, sizeof(this->addr)) == SOCKET_ERROR) {
            std::cerr << "Error binding socket to port " << this->bank_port
                      << ": " << WSAGetLastError() << std::endl;
            closesocket(this->sock);
        }
        else
            // ��ӡ�󶨳ɹ��Ķ˿ں�
            std::cout << "Successfully bound to port: " << this->bank_port << std::endl;

    };


    //getter&setter
    int get_balance() const;
    void set_balance(int balance);

    int get_port() const;

    int get_node_state() const;

    bool get_snapshot_flag() const;
    void set_snapshot_flag(bool flag);
    //send transfer money
    void send_transfer(int id, int amout, std::time_t timestamp);

    //receive transfer money
    void receive_transfer();

    //broadcast shapshot request
    void snapshot();

    //record self state(balance) when take snapshot
    int record_node();
    void record_channel();

    //display all process
    void display();

    ~bank();
    signals:
    void send_snapshot(int source, int target, time_t timestamp);
private:
    int balance;
    int node_state;
    int bank_id;
    int bank_port;
    SOCKET sock;
    sockaddr_in addr;
    std::string multicast_group;
    std::mutex message_mutex;
    bool first_time_snapshot = false;
};


#endif //BANK_CHANDY_LAMPORT_BANK_H
