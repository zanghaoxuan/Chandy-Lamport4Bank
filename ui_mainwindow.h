/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.8.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QPushButton *AddNode;
    QPushButton *GetSnapshot;
    QPushButton *StartTransfer;
    QTableWidget *TransferTbale;
    QTableWidget *BalanceTable;
    QMenuBar *menubar;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(629, 418);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        AddNode = new QPushButton(centralwidget);
        AddNode->setObjectName("AddNode");
        AddNode->setGeometry(QRect(530, 320, 91, 41));
        GetSnapshot = new QPushButton(centralwidget);
        GetSnapshot->setObjectName("GetSnapshot");
        GetSnapshot->setGeometry(QRect(400, 320, 91, 41));
        StartTransfer = new QPushButton(centralwidget);
        StartTransfer->setObjectName("StartTransfer");
        StartTransfer->setGeometry(QRect(270, 320, 91, 41));
        TransferTbale = new QTableWidget(centralwidget);
        if (TransferTbale->columnCount() < 4)
            TransferTbale->setColumnCount(4);
        QTableWidgetItem *__qtablewidgetitem = new QTableWidgetItem();
        TransferTbale->setHorizontalHeaderItem(0, __qtablewidgetitem);
        QTableWidgetItem *__qtablewidgetitem1 = new QTableWidgetItem();
        TransferTbale->setHorizontalHeaderItem(1, __qtablewidgetitem1);
        QTableWidgetItem *__qtablewidgetitem2 = new QTableWidgetItem();
        TransferTbale->setHorizontalHeaderItem(2, __qtablewidgetitem2);
        QTableWidgetItem *__qtablewidgetitem3 = new QTableWidgetItem();
        TransferTbale->setHorizontalHeaderItem(3, __qtablewidgetitem3);
        TransferTbale->setObjectName("TransferTbale");
        TransferTbale->setGeometry(QRect(0, 10, 601, 221));
        TransferTbale->setMaximumSize(QSize(16777215, 16777215));
        BalanceTable = new QTableWidget(centralwidget);
        if (BalanceTable->columnCount() < 2)
            BalanceTable->setColumnCount(2);
        QTableWidgetItem *__qtablewidgetitem4 = new QTableWidgetItem();
        BalanceTable->setHorizontalHeaderItem(0, __qtablewidgetitem4);
        QTableWidgetItem *__qtablewidgetitem5 = new QTableWidgetItem();
        BalanceTable->setHorizontalHeaderItem(1, __qtablewidgetitem5);
        BalanceTable->setObjectName("BalanceTable");
        BalanceTable->setGeometry(QRect(10, 240, 211, 131));
        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName("menubar");
        menubar->setGeometry(QRect(0, 0, 629, 33));
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName("statusbar");
        MainWindow->setStatusBar(statusbar);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "MainWindow", nullptr));
        AddNode->setText(QCoreApplication::translate("MainWindow", "Add Node", nullptr));
        GetSnapshot->setText(QCoreApplication::translate("MainWindow", "snapshot", nullptr));
        StartTransfer->setText(QCoreApplication::translate("MainWindow", "Start", nullptr));
        QTableWidgetItem *___qtablewidgetitem = TransferTbale->horizontalHeaderItem(0);
        ___qtablewidgetitem->setText(QCoreApplication::translate("MainWindow", "Source", nullptr));
        QTableWidgetItem *___qtablewidgetitem1 = TransferTbale->horizontalHeaderItem(1);
        ___qtablewidgetitem1->setText(QCoreApplication::translate("MainWindow", "Destination", nullptr));
        QTableWidgetItem *___qtablewidgetitem2 = TransferTbale->horizontalHeaderItem(2);
        ___qtablewidgetitem2->setText(QCoreApplication::translate("MainWindow", "Amounts", nullptr));
        QTableWidgetItem *___qtablewidgetitem3 = TransferTbale->horizontalHeaderItem(3);
        ___qtablewidgetitem3->setText(QCoreApplication::translate("MainWindow", "Send Time", nullptr));
        QTableWidgetItem *___qtablewidgetitem4 = BalanceTable->horizontalHeaderItem(0);
        ___qtablewidgetitem4->setText(QCoreApplication::translate("MainWindow", "PC id", nullptr));
        QTableWidgetItem *___qtablewidgetitem5 = BalanceTable->horizontalHeaderItem(1);
        ___qtablewidgetitem5->setText(QCoreApplication::translate("MainWindow", "Balance", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
