//
// Created by 13520 on 2024/12/15.
//

#include "bank.h"
#include "bank_data.h"

#define MULTICAST_GROUP "224.0.0.1"
#define MULTICAST_PORT 5007
#define BUFFER_SIZE 1024

int bank::get_bank_id() const{
    return bank_id;
}
void bank::set_bank_id(int bankId){
    bank_id = bankId;
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

void bank::setup_multicast_receiver(){
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2,2), &wsaData);

    multicast_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    int reuse = 1;
    setsockopt(multicast_sock, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse));

    sockaddr_in local_addr;
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(MULTICAST_PORT);
    local_addr.sin_addr.s_addr = INADDR_ANY;

    bind(multicast_sock, (sockaddr*)&local_addr, sizeof(local_addr));

    ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr(MULTICAST_GROUP);
    mreq.imr_interface.s_addr = INADDR_ANY;
    setsockopt(multicast_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&mreq, sizeof(mreq));

}

//暂存部分已发送数据，根据time值判断是否在
void bank::send_transfer(int id, int amount, std::time_t timestamp) {
    WSADATA wsaData;
    SOCKET sock;
    sockaddr_in server_addr;
    char buffer[1024];
    //通过id找port
    int port = Id2Port[id];
    //初始化socket
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    //将对象序列化并发送
    bank_data data(amount);
    data.set_time(std::chrono::system_clock::from_time_t(timestamp));
    std::string message = data.serialize();

    int sent_bytes = sendto(sock, message.c_str(), message.size(), 0, (sockaddr*)&server_addr, sizeof(server_addr));
    if (sent_bytes == SOCKET_ERROR) {
        std::cerr << "sendto failed! Error: " << WSAGetLastError() << std::endl;
    } else {
        std::cout << "Sent message: " << message <<" From "<<this->bank_port<<" To "<<ntohl(server_addr.sin_port)<<std::endl;
        this->set_balance(this->balance-amount);
    }
    //closesocket(sock);
    //WSACleanup();
}


void bank::receive_transfer() {
    WSADATA wsaData;
    SOCKET sock;
    sockaddr_in server_addr, client_addr;
    char buffer[1024];
    int client_len = sizeof(client_addr);
    memset(&server_addr, 0, sizeof(server_addr));

    // 初始化 WinSock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed!" << std::endl;
        return;
    }

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Socket creation failed! Error: " << WSAGetLastError() << std::endl;
        return;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(this->bank_port);
    std::cout<<"port"<<this->bank_port<<std::endl;
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed!" << std::endl;
        closesocket(sock);
        WSACleanup();
        return;
    }

    while (true) {
        // 接收数据
        memset(buffer, 0, sizeof(buffer));
        int len = recvfrom(sock, buffer, sizeof(buffer) - 1, 0, nullptr, nullptr);
        if (len == SOCKET_ERROR) {
            if (WSAGetLastError() == WSAEWOULDBLOCK) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            } else {
                std::cerr << "Error receiving message: " << WSAGetLastError() << std::endl;
                continue;
            }
        }
        buffer[len] = '\0';
        std::string message(buffer);

        // 输出接收到的消息
        std::cout << "Received message: " << buffer << std::endl;

        // 反序列化数据
        try {
            std::string received_message(buffer);
            bank_data received_data = bank_data::deserialize(received_message);

            // 检查转账金额
            int transfer_amount = received_data.get_transfer();
            auto timestamp = received_data.get_time();
            if (snapshot_flag) {
                // 如果是快照信号
                std::lock_guard<std::mutex> lock(message_mutex);
                inbound_messages.push_back(message);
                std::cout << "Message added to inbound queue: " << message << std::endl;
            }

            // 否则，更新余额
            this->set_balance(this->balance + transfer_amount);
            std::cout << "Transfer received: " << transfer_amount << ", new balance: " << this->balance << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error during deserialization: " << e.what() << std::endl;
        }
    }

    // 清理资源
    closesocket(sock);
    WSACleanup();
}


//
void bank::snapshot() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    multicast_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    int reuse = 1;
    setsockopt(multicast_sock, SOL_SOCKET, SO_REUSEADDR, (char *) &reuse, sizeof(reuse));

    sockaddr_in local_addr;
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = ntohs(MULTICAST_PORT);
    local_addr.sin_addr.s_addr = INADDR_ANY;

    bank_data data(0);
    std::string message = data.serialize();
    int message_len = message.size();

    sendto(multicast_sock, message.c_str(), message_len, 0, (sockaddr *) &local_addr, sizeof(local_addr));

    closesocket(multicast_sock);
    WSACleanup();
}

int bank::record(){
    // 保存当前余额
    int current_balance = this->get_balance();
    std::cout << "Recording snapshot: Bank ID " << this->bank_id
              << ", Balance: " << current_balance << std::endl;

    // 保存入站消息
    std::lock_guard<std::mutex> lock(message_mutex);
    std::cout << "Inbound messages during snapshot:" << std::endl;
    for (const auto &message : inbound_messages) {
        try {
            bank_data data = bank_data::deserialize(message);
            std::cout << " - Amount=" << data.get_transfer()
                      << ", Timestamp=" << std::chrono::system_clock::to_time_t(data.get_time()) << std::endl;
        } catch (const std::exception &e) {
            std::cerr << "Error deserializing inbound message: " << e.what() << std::endl;
        }
    }

    // 清空入站队列
    inbound_messages.clear();

    // 结束快照
    snapshot_flag = false;
}

bank::~bank(){
    closesocket(multicast_sock);
    WSACleanup();
}