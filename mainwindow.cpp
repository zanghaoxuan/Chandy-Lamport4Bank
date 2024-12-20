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

//银行增加
bool add_bank(int id, int balance) {
    // 检查是否已经存在
    if (Banks.find(id) != Banks.end()) {
        std::cerr << "Bank with ID " << id << " already exists!" << std::endl;
        return false;
    }

    // 添加新的 Bank 对象
    Banks.emplace(std::piecewise_construct,
                  std::forward_as_tuple(bank_number),
                  std::forward_as_tuple());
    Id2Port.emplace(bank_number, Banks[bank_number].get_port());
    Banks[bank_number].set_balance(balance);
    std::cout << "Bank with ID " << id << " added successfully." << std::endl;

    return true;
}



MainWindow::MainWindow(QWidget *parent) :
        QMainWindow(parent),
        ui(new Ui::MainWindow),
        snapshotFlag(false),
        transferTimer(new QTimer(this)) {
    ui->setupUi(this);
    // 增加新银行
    connect(ui->AddNode, &QPushButton::clicked, this, &MainWindow::Clicked_AddPC);
    // 开始转账
    connect(ui->StartTransfer, &QPushButton::clicked, this, &MainWindow::Clicked_StartTransfer);
    // 开始快照
    connect(ui->GetSnapshot, &QPushButton::clicked, this, &MainWindow::Clicked_Snapshot);
    // 定时执行转账
    connect(transferTimer, &QTimer::timeout, this, &MainWindow::PerformTransfer);

    std::srand(std::time(nullptr));
}
//增加新银行
void MainWindow::Clicked_AddPC() {
    //随机金额
    int balance = rand() % 100000 + 1000;
    add_bank(bank_number, balance);
    std::thread t(&bank::receive_transfer, &Banks[bank_number]);
    t.detach();
    //Id2Thread[bank_number] = std::move(t);

    int row = ui->BalanceTable->rowCount();
    ui->BalanceTable->insertRow(row);
    ui->BalanceTable->setItem(row, 0, new QTableWidgetItem(QString::number(bank_number)));
    ui->BalanceTable->setItem(row, 1, new QTableWidgetItem(QString::number(balance)));
    bank_number++;
}
void MainWindow::Clicked_StartTransfer() {
        std::cout << "Start Transfer" << std::endl;
        transferTimer->start(500); // 每0.5秒执行一次
}


void MainWindow::PerformTransfer() {
    std::lock_guard<std::mutex> lock(banksMutex); // 确保线程安全
    if (snapshot_flag) {
        transferTimer->stop(); // 停止转账
        return;
    }

    // 随机生成转账方和接收方
    int random_bank_resource, random_bank_target;
    do {
        random_bank_resource = 1 + rand() % (bank_number - 1);
    } while (Banks[random_bank_resource].get_balance() == 0);

    do {
        random_bank_target = 1 + rand() % (bank_number - 1);
    } while (random_bank_target == random_bank_resource);

    int resource_balance = Banks[random_bank_resource].get_balance();
    int amount = 1 + rand() % (resource_balance - 1);

    std::time_t timestamp = std::time(nullptr);
    Banks[random_bank_resource].send_transfer(random_bank_target, amount, timestamp);

    // 更新 UI 表格
    int row = ui->TransferTbale->rowCount();
    ui->TransferTbale->insertRow(row);
    ui->TransferTbale->setItem(row, 0, new QTableWidgetItem(QString::number(random_bank_resource)));
    ui->TransferTbale->setItem(row, 1, new QTableWidgetItem(QString::number(random_bank_target)));
    ui->TransferTbale->setItem(row, 2, new QTableWidgetItem(QString::number(amount)));
    ui->TransferTbale->setItem(row, 3, new QTableWidgetItem(QString::fromStdString(std::ctime(&timestamp))));
    ui->TransferTbale->resizeColumnToContents(3);
    ui->TransferTbale->update();
}
void MainWindow::Clicked_Snapshot() {
    std::lock_guard<std::mutex> lock(banksMutex);
    snapshotFlag = true;

    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    int table_row = ui->TransferTbale->rowCount();
    ui->TransferTbale->insertRow(table_row);
    ui->TransferTbale->setItem(table_row, 0, new QTableWidgetItem(QString::number(-1)));
    ui->TransferTbale->setItem(table_row, 1, new QTableWidgetItem(QString::number(-1)));
    ui->TransferTbale->setItem(table_row, 2, new QTableWidgetItem(QString::number(0)));
    ui->TransferTbale->setItem(table_row, 3, new QTableWidgetItem(QString::fromStdString(std::ctime(&now_c))));
    ui->TransferTbale->resizeColumnToContents(3);
    ui->TransferTbale->update();

    for(int col = 0; col <  ui->TransferTbale->columnCount(); ++col){
        ui->TransferTbale->item(table_row, col)->setBackground(QColor(255, 0, 0));
    }

    // 广播快照信号
    for (auto &entry : Banks) {
        entry.second.snapshot();
    }

    // 更新余额表格
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
    ui->BalanceTable->setItem(row, 0, new QTableWidgetItem("Total"));
    int total_balance = 0;
    for (const auto &entry : Banks) {
        total_balance += entry.second.get_balance();
    }
    ui->BalanceTable->setItem(row, 1, new QTableWidgetItem(QString::number(total_balance)));
    snapshotFlag = false;
}


MainWindow::~MainWindow() {
    delete ui;
}
