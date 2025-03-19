//
// Created by 13520 on 2024/12/16.
//

#ifndef BANK_MAINWINDOW_H
#define BANK_MAINWINDOW_H

#include <iostream>
#include <map>
#include "bank.h"
#include <thread>
#include <atomic>
#include <QMainWindow>
#include <QPushButton>
#include <QTabWidget>
#include <QWidget>
#include <QTableWidgetItem>
#include <pthread.h>
#include <QTimer>
#include <mutex>
#include <QObject>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    bool  add_bank(int id, int balance);

    ~MainWindow() override;

public slots:
    void Clicked_AddPC();
    void Clicked_StartTransfer();
    void Clicked_Snapshot();
    void PerformTransfer();
    void UpdateSnapTable(int source, int target, time_t timestamp);
    void UpdataBalanceTbale();

private:
    Ui::MainWindow *ui;
    QTimer *transferTimer; // 定时器
    std::mutex banksMutex; // 用于线程安全的互斥锁
    std::atomic<bool> snapshotFlag; // 快照触发标志
    QTimer *snapshotCheckTimer; // 定时器用于检查快照完成状态
    void checkSnapshotCompletion(); // 检查快照完成状态
};


#endif //BANK_MAINWINDOW_H
