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
bool bank::send_transfer(int id, int amout, std::string &timestamp) {
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

    //bind(sock, (sockaddr*)&server_addr,  sizeof(server_addr));
    //将对象序列化并发送
    bank_data data(amout);
    std::string message = data.serialize();
    //std::cout<<"message"<<message<<"port"<<server_addr.sin_port<<std::endl;
    //std::cout<<"send_transfer send port"<<htons(this->bank_port)<<" to port"<<server_addr.sin_port<<std::endl;
    int sent_bytes = sendto(sock, message.c_str(), message.size(), 0, (sockaddr*)&server_addr, sizeof(server_addr));
    if (sent_bytes == SOCKET_ERROR) {
        std::cerr << "sendto failed! Error: " << WSAGetLastError() << std::endl;
    } else {
        std::cout << "Sent message: " << message <<" From "<<this->bank_port<<" To "<<ntohl(server_addr.sin_port)<<std::endl;
    }
    //获取发送的时间戳
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    timestamp = std::ctime(&now_time);
    //接受ack,调整余额
    socklen_t server_addr_len = sizeof(server_addr);
    int recv_len = recvfrom(sock, buffer, sizeof(buffer) - 1, 0, (sockaddr*)&server_addr, &server_addr_len);
    buffer[recv_len] = '\0';
    //std::cout<<"buffer"<<buffer<<std::endl;
    if(buffer == "ACK: "+message) {
        this->set_balance(this->balance - data.get_transfer());
        closesocket(sock);
        WSACleanup();
        return true;
    }

    closesocket(sock);
    WSACleanup();
    return false;
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
        //std::cout << "Waiting for data on port " << ntohs(server_addr.sin_port) << std::endl;
        //std::cout << "Socket before recvfrom: " << sock << std::endl;
        int len = recvfrom(sock, buffer, sizeof(buffer) - 1, 0, nullptr, nullptr);
        if (len == SOCKET_ERROR) {
            //std::cout << "Socket before recvfrom: " << sock << std::endl;
            //std::cerr << "recvfrom failed! Error: " << WSAGetLastError() << std::endl;
            continue;
        }
        buffer[len] = '\0';


        // 输出接收到的消息
        std::cout << "Received message: " << buffer << std::endl;

        // 反序列化数据
        try {
            std::string received_message(buffer);
            bank_data received_data = bank_data::deserialize(received_message);

            // 检查转账金额
            float transfer_amount = received_data.get_transfer();
            if (transfer_amount == 0) {
                // 如果是快照信号
                this->snapshot_flag = true;

                auto time = received_data.get_time();
                std::time_t t = std::chrono::system_clock::to_time_t(time);
                std::cout << "Received snapshot at: " << std::ctime(&t) << std::endl;

                int snapshot_balance = this->record();
                std::cout << "Snapshot balance of node " << this->get_bank_id() << " is: " << snapshot_balance << std::endl;
                break; // 退出循环，不再接收数据
            }

            // 否则，更新余额
            this->set_balance(this->balance + transfer_amount);
            std::cout << "Transfer received: " << transfer_amount << ", new balance: " << this->balance << std::endl;

            // 发送 ACK
            std::string ack_message = "ACK: " + std::string(buffer);
            std::cout<<"ack_message"<<ack_message<<std::endl;
            sendto(sock, ack_message.c_str(), ack_message.size(), 0, (sockaddr*)&client_addr, client_len);

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
    int balanbce = this->balance;
    return balanbce;
}

bank::~bank(){
    closesocket(multicast_sock);
    WSACleanup();
}