//
// Created by 13520 on 2024/12/15.
//

#include <QCoreApplication>
#include "bank.h"
#include "bank_data.h"

extern std::map<int, bank>Banks;

bool bank::get_snapshot_flag() const {
    return first_time_snapshot;
}
void bank::set_snapshot_flag(bool flag) {
    this->first_time_snapshot = flag;
}

int bank:: get_balance() const{
    return balance;
}
void bank::set_balance(int balance){
    this->balance = balance;
}

int bank::get_port() const{
    return this->bank_port;
}

int bank::get_node_state() const {
    return node_state;
}

//暂存部分已发送数据，根据time值判断是否在
void bank::send_transfer(int id, int amount, std::time_t timestamp) {
    sockaddr_in target_addr;
    char buffer[1024];
    //通过id找port
    int port = Id2Port[id];

    memset(&target_addr, 0, sizeof(target_addr));
    target_addr.sin_family = AF_INET;
    target_addr.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &target_addr.sin_addr);

    //将对象序列化并发送
    bank_data data(amount);
    data.set_time(std::chrono::system_clock::from_time_t(timestamp));
    std::string message = data.serialize();

    int sent_bytes = sendto(sock, message.c_str(), message.size(), 0, (sockaddr*)&target_addr, sizeof(target_addr));
    if (sent_bytes == SOCKET_ERROR) {
        std::cerr << "sendto failed! Error: " << WSAGetLastError() << std::endl;
    } else {
        std::cout << "Sent message: " << message <<" From "<<this->bank_port<<" To "<<ntohl(target_addr.sin_port)<<std::endl;
        this->set_balance(this->balance-amount);
    }
}


void bank::receive_transfer() {
    sockaddr_in send_addr;
    char buffer[1024];
    int send_len = sizeof(send_addr);
    memset(&this->addr, 0, sizeof(this->addr));

    while (true) {
        // 接收数据
        memset(buffer, 0, sizeof(buffer));
        int len = recvfrom(sock, buffer, sizeof(buffer) - 1, 0, reinterpret_cast<sockaddr *>(&send_addr), &send_len);
        if (len == SOCKET_ERROR) {
            continue;
        }
        std::string message(buffer);
        std::string snapshot_message = message.substr(0, 8);

        //获取消息发送者的id
        int source_bank_port = ntohs(send_addr.sin_port);
        int source_bank_id;
        for(auto &i : Id2Port){
            if(i.second == source_bank_port){
                source_bank_id = i.first;
            }
        }

        //如果数据是快照消息
        if(snapshot_message == "SNAPSHOT"){
            //第一次接到快照消息
            std::cout<<source_bank_id<<" "<<this->bank_id<<"snapshot"<<std::endl;
            if(!first_time_snapshot){
                first_time_snapshot = true;
                this->record_node();
                this->record_channel();
                //将发送快照消息的邻居标记
               channelSnapshotCompleted[this->bank_id][source_bank_id] = true;
                // 将所有未完成的快照标志设置为 false
                for (const auto &neighbor : Id2Port) {
                    if (neighbor.first != this->bank_id) {
                        channelSnapshotCompleted[this->bank_id][neighbor.first] = false;
                    }
                }
                //记录本届点进入快照状态的时间
                auto now = std::chrono::system_clock::now();
                std::time_t now_c = std::chrono::system_clock::to_time_t(now);
                SnapshotTime[this->bank_id] = now_c;
                //继续向下洪范
                this->snapshot();
            }
        }else{

            bank_data received_data = bank_data::deserialize(message);

            // 检查转账金额
            int transfer_amount = received_data.get_transfer();
            //转账发出的时间
            auto time = received_data.get_time();
            std::time_t send_time = std::chrono::system_clock::to_time_t(time);

            this->set_balance(this->balance + transfer_amount);
            std::cout << "Transfer received: " << transfer_amount << ", new balance: " << this->balance << std::endl;
            //获取现在时间
            auto now = std::chrono::system_clock::now();
            time_t recieve_time = std::chrono::system_clock::to_time_t(now);
            inboundChannels[source_bank_id].emplace(transfer_amount, std::map<time_t, time_t>{{send_time, recieve_time}});//<发送方, 金额 <<发送时间，接收时间>>>
        }
    }
    // 清理资源
    closesocket(sock);
    WSACleanup();
}


//
void bank::snapshot() {
    std::string snapshot_message = "SNAPSHOT";


    for (int neighbor = 1; neighbor < bank_number; neighbor++) {
        if(neighbor != this->bank_id) {
            // 接收方地址
            sockaddr_in target_addr;
            memset(&target_addr, 0, sizeof(target_addr));
            target_addr.sin_family = AF_INET;
            target_addr.sin_port = htons(Id2Port[neighbor]);
            inet_pton(AF_INET, "127.0.0.1", &target_addr.sin_addr);

            // 发送消息
            sendto(sock, snapshot_message.c_str(), snapshot_message.size(), 0, (sockaddr *) &target_addr,sizeof(target_addr));
            std::cout << "Snapshot message sent to bank " << neighbor << std::endl;

            // 发射信号
            time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            emit send_snapshot(this->bank_id, neighbor, now);

            // 等待 1 秒
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}


int bank::record_node(){
    // 保存当前余额
    this->node_state = this->get_balance();
    std::cout << "Recording snapshot: Bank ID " << this->bank_id
              << ", Balance: " << node_state << std::endl;
    return this->node_state;
}

void bank::record_channel() {
    //<发送方, <金额，<发送时间，接收时间>>
    for(const auto& [send_id, channel_data] : inboundChannels){
        for(const auto& [value, time_data] : channel_data){
            for(const auto& [send_time, receive_time] : time_data){
                //发送者发出时未进入快照状态，接收者收到时已进入快照状态,this是接收者
                if(send_time < SnapshotTime[send_id] && receive_time > SnapshotTime[this->bank_id]){
                    if(send_time > start_snapshot_time &&  receive_time > start_snapshot_time)
                        ChannelState[this->bank_id][send_id] += value;//接收方->发出方->金额
                }
            }
        }
    }
}
bank::~bank(){
    closesocket(sock);
    WSACleanup();
}