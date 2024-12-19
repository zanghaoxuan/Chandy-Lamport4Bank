//
// Created by 13520 on 2024/12/16.
//

// You may need to build the project (run Qt uic code generator) to get "ui_MainWindow.h" resolved
#include "mainwindow.h"
#include "ui_MainWindow.h"

int bank_number = 1;
std::map<int, int> Id2Port;
std::map<int, bank> Banks;
//std::map<int, std::thread> Id2Thread;
std::atomic<bool> snapshot_flag(false);

//��������
bool add_bank(int id, int balance) {
    // ����Ƿ��Ѿ�����
    if (Banks.find(id) != Banks.end()) {
        std::cerr << "Bank with ID " << id << " already exists!" << std::endl;
        return false;
    }

    // ����µ� Bank ����
    Banks.emplace(id, bank());
    Id2Port.emplace(bank_number, Banks[bank_number].get_port());
    Banks[bank_number].set_balance(balance);
    std::cout << "Bank with ID " << id << " added successfully." << std::endl;
    return true;
}



MainWindow::MainWindow(QWidget *parent) :
        QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    transferTimer = new QTimer(this); // ��ʼ����ʱ��
    // ����������
    connect(ui->AddNode, &QPushButton::clicked, this, &MainWindow::Clicked_AddPC);
    // ��ʼת��
    connect(ui->StartTransfer, &QPushButton::clicked, this, &MainWindow::Clicked_StartTransfer);
    // ��ʼ����
    connect(ui->GetSnapshot, &QPushButton::clicked, this, &MainWindow::Clicked_Snapshot);
    // ��ʱִ��ת��
    connect(transferTimer, &QTimer::timeout, this, &MainWindow::PerformTransfer);
}
//����������
void MainWindow::Clicked_AddPC() {
    //������
    int balance = rand() % 100000 + 1000;
    add_bank(bank_number, balance);
    std::thread t(&bank::receive_transfer, &Banks[bank_number]);
    /*
    std::thread t([=]() {
        std::string threadName = "Bank" + std::to_string(bank_number);
        std::cout << "Thread started: " << threadName << std::endl;
        Banks[bank_number].receive_transfer();
    });
     */
    t.detach();
    //Id2Thread[bank_number] = std::move(t);
    bank_number++;
}
void MainWindow::Clicked_StartTransfer() {
    std::cout << "Start Transfer" << std::endl;
    transferTimer->start(2000); // ÿ2��ִ��һ��
}


void MainWindow::PerformTransfer() {
    std::lock_guard<std::mutex> lock(banksMutex); // ȷ���̰߳�ȫ
    if (snapshot_flag) {
        transferTimer->stop(); // ֹͣת��
        return;
    }

    // �������ת�˷��ͽ��շ�
    int random_bank_resource, random_bank_target;
    do {
        random_bank_resource = 1 + rand() % (bank_number - 1);
    } while (Banks[random_bank_resource].get_balance() == 0);

    do {
        random_bank_target = 1 + rand() % (bank_number - 1);
    } while (random_bank_target == random_bank_resource);

    int resource_balance = Banks[random_bank_resource].get_balance();
    int amount = 1 + rand() % (resource_balance - 1);

    std::string timestamp;
    bool ack = Banks[random_bank_resource].send_transfer(random_bank_target, amount, timestamp);

    // ���� UI ���
    int row = ui->TransferTbale->rowCount();
    ui->TransferTbale->insertRow(row);
    ui->TransferTbale->setItem(row, 0, new QTableWidgetItem(QString::number(random_bank_resource)));
    ui->TransferTbale->setItem(row, 1, new QTableWidgetItem(QString::number(random_bank_target)));
    ui->TransferTbale->setItem(row, 2, new QTableWidgetItem(QString::number(amount)));
    ui->TransferTbale->setItem(row, 3, new QTableWidgetItem(QString::fromStdString(timestamp)));

    if (ack) {
        for (int col = 0; col < 4; col++) {
            ui->TransferTbale->item(row, col)->setBackground(Qt::green);
        }
    }
    ui->TransferTbale->update();
}
void MainWindow::Clicked_Snapshot() {
    std::lock_guard<std::mutex> lock(banksMutex);
    int random_bank = 1 + rand() % (bank_number - 1);
    Banks[random_bank].snapshot();
    snapshot_flag = true;

    // ���������
    int row = 0;
    ui->BalanceTable->setRowCount(Banks.size());
    for (const auto &entry : Banks) {
        int bank_id = entry.first;
        int bank_balance = entry.second.get_balance();

        QTableWidgetItem *idItem = new QTableWidgetItem(QString::number(bank_id));
        ui->BalanceTable->setItem(row, 0, idItem);

        QTableWidgetItem *balanceItem = new QTableWidgetItem(QString::number(bank_balance));
        ui->BalanceTable->setItem(row, 1, balanceItem);

        row++;
    }
}

MainWindow::~MainWindow() {
    delete ui;
}
