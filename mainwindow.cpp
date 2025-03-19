//
// Created by 13520 on 2024/12/16.
//

// You may need to build the project (run Qt uic code generator) to get "ui_MainWindow.h" resolved
#include "mainwindow.h"
#include "ui_MainWindow.h"

int bank_number = 1;
std::map<int, int> Id2Port;
std::map<int, bank> Banks;
std::atomic<bool> snapshot_flag(false);
std::map<int, time_t> SnapshotTime;//记录每个节点进入快照的时间，第一次收到快照消息的时间
std::map<int, std::map<int, int>> ChannelState;//信道上的消息状态，<接收方,发出方>,金额
std::map<int, std::map<int, bool>> channelSnapshotCompleted;// 标记每个通道（邻居）的快照状态
time_t start_snapshot_time;

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
    //检查是否全部完成快照
    snapshotCheckTimer = new QTimer(this);//控制检查是否全完成快照时间
    connect(snapshotCheckTimer, &QTimer::timeout, this, &MainWindow::checkSnapshotCompletion);
    std::srand(std::time(nullptr));//控制转账的时间
}
//银行增加
bool  MainWindow::add_bank(int id, int balance) {
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
    bool connected = connect(&Banks[bank_number], &bank::send_snapshot, this, &MainWindow::UpdateSnapTable);
    qDebug() << "Connection status: " << connected;
    return true;
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
    int row = ui->BalanceTable->rowCount();
    ui->BalanceTable->insertRow(row);
    ui->BalanceTable->setItem(row, 0, new QTableWidgetItem("Total"));
    int total_balance = 0;
    for (const auto &entry: Banks) {
        total_balance += entry.second.get_balance();
    }
    ui->BalanceTable->setItem(row, 1, new QTableWidgetItem(QString::number(total_balance)));

    std::cout << "Start Transfer" << std::endl;
    transferTimer->start(100); // 每0.5秒执行一次
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
void MainWindow::UpdateSnapTable(int source, int target, time_t timestamp) {
    // 转换时间戳
    QString timeStr = QString::fromStdString(std::ctime(&timestamp));
    int table_row = ui->TransferTbale->rowCount();
    ui->TransferTbale->insertRow(table_row);
    ui->TransferTbale->setItem(table_row, 0, new QTableWidgetItem(QString::number(source)));
    ui->TransferTbale->setItem(table_row, 1, new QTableWidgetItem(QString::number(target)));
    ui->TransferTbale->setItem(table_row, 2, new QTableWidgetItem("SNAPSHOT"));
    ui->TransferTbale->setItem(table_row, 3, new QTableWidgetItem(timeStr));
    ui->TransferTbale->resizeColumnToContents(3);
    ui->TransferTbale->update();
    //将改行变成红色

    for (int i = 0; i < ui->TransferTbale->columnCount(); ++i) {
        QTableWidgetItem *item = ui->TransferTbale->item(table_row, i);
        if (item) {
            item->setBackground(Qt::red);
        }
    }

}

void MainWindow::Clicked_Snapshot() {
    //复原所有的状态容器到空
    SnapshotTime.clear();
    ChannelState.clear();
    channelSnapshotCompleted.clear();
    for(auto &[id, bank]:Banks){
        bank.inboundChannels.clear();
        bank.set_snapshot_flag(false);
        for(int neighbor = 1; neighbor < bank_number; neighbor++){
            if(neighbor != id)
                ChannelState[neighbor][id] = 0;
        }
    }

    std::lock_guard<std::mutex> lock(banksMutex);
    snapshotFlag = true;

    //随机挑选节点发送快照信号
    int random_bank_resource, random_bank_target;
    random_bank_resource = 1 + rand() % (bank_number - 1);
    do {
        random_bank_target = 1 + rand() % (bank_number - 1);
    } while (random_bank_target == random_bank_resource);
    //记录发送者的快照状态进入时间
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    start_snapshot_time = now_c;
    SnapshotTime[random_bank_resource] = now_c;
    //发送快照消息
    Banks[random_bank_resource].set_snapshot_flag(true);
    Banks[random_bank_resource].record_node();
    Banks[random_bank_resource].snapshot();


    // 启动定时器，每 500 毫秒检查一次,检查是否全部进入快照
    snapshotCheckTimer->start(500);

}

void MainWindow::checkSnapshotCompletion() {
    bool allCompleted = true;

    // 遍历所有银行，检查 flag 状态
    for (const auto &entry : Banks) {
        if (!entry.second.get_snapshot_flag()) { // 假设 get_flag() 返回快照完成状态
            allCompleted = false;
            break;
        }
    }

    if (allCompleted) {
        snapshotCheckTimer->stop(); // 停止检查定时器
        UpdataBalanceTbale();    // 更新表格
        qDebug() << "All banks completed snapshot!";
    }
}

void MainWindow::UpdataBalanceTbale() {
    // 清空表格
    ui->BalanceTable->clearContents();
    ui->BalanceTable->setRowCount(0);
    //ui->BalanceTable->setRowCount(Banks.size());


    // 更新余额表格
    int row = 0;
    //ui->BalanceTable->setRowCount(Banks.size());
    for (const auto &entry: Banks) {
        int bank_id = entry.first;
        int bank_balance = entry.second.get_node_state();
        ui->BalanceTable->insertRow(row);
        QTableWidgetItem *idItem = new QTableWidgetItem(QString::number(bank_id));
        ui->BalanceTable->setItem(row, 0, idItem);

        QTableWidgetItem *balanceItem = new QTableWidgetItem(QString::number(bank_balance));
        ui->BalanceTable->setItem(row, 1, balanceItem);

        row++;
    }
    for (auto &entry: ChannelState) {
        for (const auto &innerEntry: entry.second) {
            int recivev = entry.first;
            int send = innerEntry.first;
            int balance = innerEntry.second;
            std::string str = std::to_string(send) + "->" + std::to_string(recivev);
            ui->BalanceTable->insertRow(row);
            QTableWidgetItem *idItem = new QTableWidgetItem(QString::fromStdString(str));
            ui->BalanceTable->setItem(row, 0, idItem);

            QTableWidgetItem *balanceItem = new QTableWidgetItem(QString::number(balance));
            ui->BalanceTable->setItem(row, 1, balanceItem);
            row++;
        }
    }
    ui->BalanceTable->insertRow(row);
    ui->BalanceTable->setItem(row, 0, new QTableWidgetItem("Total"));
    //计算该表所有项的总余额
    int total_balance = 0;
    for(int i = 0; i < row-1; i++){
        total_balance += ui->BalanceTable->item(i, 1)->text().toInt();
    }

    ui->BalanceTable->setItem(row, 1, new QTableWidgetItem(QString::number(total_balance)));


    ui->BalanceTable->update();
    ui->BalanceTable->repaint();
}

MainWindow::~MainWindow() {
    delete ui;
}
