#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QMessageBox>
#include <QStandardItemModel>
#include <QBluetoothDeviceInfo>
#include <QBluetoothServiceInfo>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , blueDevice(new BlueDevice(this))
{
    ui->setupUi(this);

    // 设置设备发现的列表视图
    QStandardItemModel *deviceModel = new QStandardItemModel(this);
    ui->deviceListView->setModel(deviceModel);

    // 连接设备的信号槽
    connect(ui->startDiscoveryButton, &QPushButton::clicked, this, &MainWindow::onStartDiscoveryClicked);
    connect(ui->stopDiscoveryButton, &QPushButton::clicked, this, &MainWindow::onStopDiscoveryClicked); // 连接停止搜索按钮
    connect(ui->deviceListView->selectionModel(), &QItemSelectionModel::currentChanged, this, &MainWindow::onDeviceSelected);
    connect(ui->serviceComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onServiceSelected);
    connect(ui->sendDataButton, &QPushButton::clicked, this, &MainWindow::onSendDataClicked);
    connect(ui->disconnectButton, &QPushButton::clicked, this, &MainWindow::onDisconnectButtonClicked); // 断开连接

    // 连接蓝牙设备相关信号
    connect(blueDevice, &BlueDevice::deviceDiscovered, this, &MainWindow::onDeviceDiscovered);
    connect(blueDevice, &BlueDevice::connectionEstablished, this, &MainWindow::onConnectionEstablished);
    connect(blueDevice, &BlueDevice::connectionLost, this, &MainWindow::onConnectionLost);
    connect(blueDevice, &BlueDevice::dataReceived, this, &MainWindow::onDataReceived);
    connect(blueDevice, &BlueDevice::socketErrorOccurred, this, &MainWindow::onSocketErrorOccurred); // 连接socket错误信号
    connect(blueDevice, &BlueDevice::bleErrorOccurred, this, &MainWindow::onBleErrorOccurred); // 连接ble错误信号

    ui->stopDiscoveryButton->setEnabled(false); // 初始状态禁用停止搜索按钮
    ui->disconnectButton->setEnabled(false); // 初始状态禁用断开连接按钮
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onStartDiscoveryClicked()
{
    ui->startDiscoveryButton->setEnabled(false);
    ui->stopDiscoveryButton->setEnabled(true);
    QStandardItemModel *model = qobject_cast<QStandardItemModel*>(ui->deviceListView->model());
    model->clear(); // 清空设备列表
    blueDevice->startDiscovery();
}

void MainWindow::onStopDiscoveryClicked()
{
    blueDevice->getDiscoveryAgent()->stop();
    ui->startDiscoveryButton->setEnabled(true);
    ui->stopDiscoveryButton->setEnabled(false);
    QMessageBox::information(this, "搜索停止", "蓝牙设备搜索已停止!");
}

void MainWindow::onDeviceSelected(const QModelIndex &index)
{
    QStandardItemModel *model = qobject_cast<QStandardItemModel*>(ui->deviceListView->model());
    if (model && index.isValid()) {
        selectedDevice = model->itemFromIndex(index)->data(Qt::UserRole).value<QBluetoothDeviceInfo>();
        // 选择设备后，启动服务发现
        blueDevice->connectToDevice(selectedDevice);
        // 更新 UI 中的服务列表
        ui->serviceComboBox->clear();
        if (selectedDevice.coreConfigurations() & QBluetoothDeviceInfo::LowEnergyCoreConfiguration) {
            ui->serviceComboBox->setEnabled(false);
        } else {
            ui->serviceComboBox->setEnabled(true);
        }
        ui->disconnectButton->setEnabled(true); // 允许断开连接
    }
}

void MainWindow::onServiceSelected(int index)
{
    if (index >= 0 && index < availableServices.size()) {
        const QBluetoothServiceInfo &service = availableServices.at(index);
        blueDevice->connectToDevice(service.device());
    }
}

void MainWindow::onSendDataClicked()
{
    QString data = ui->sendDataLineEdit->text();
    if (!data.isEmpty()) {
        blueDevice->sendData(data.toUtf8());
        ui->sendDataLineEdit->clear();
    }
}

void MainWindow::onDataReceived(const QByteArray &data)
{
    ui->receivedDataTextEdit->append("收到数据: " + QString::fromUtf8(data));
}

void MainWindow::onConnectionEstablished()
{
    QMessageBox::information(this, "连接成功", "设备连接成功!");
    ui->disconnectButton->setEnabled(true); // 允许断开连接
}

void MainWindow::onConnectionLost()
{
    QMessageBox::information(this, "连接断开", "设备连接断开!");
    ui->disconnectButton->setEnabled(false); // 禁止断开连接
}

void MainWindow::onDisconnectButtonClicked()
{
    blueDevice->disconnectDevice();
    ui->disconnectButton->setEnabled(false);
}

void MainWindow::onDeviceDiscovered(const QBluetoothDeviceInfo &device)
{
    QStandardItemModel *model = qobject_cast<QStandardItemModel*>(ui->deviceListView->model());
    if (model) {
        QList<QStandardItem*> items = model->findItems(device.address().toString(), Qt::MatchExactly, 1); // 在第二列查找
        if (items.isEmpty()) {
            QStandardItem *item = new QStandardItem(device.name());
            QStandardItem *addressItem = new QStandardItem(device.address().toString()); // 添加地址
            item->setData(QVariant::fromValue(device), Qt::UserRole);
            QList<QStandardItem*> row;
            row << item << addressItem; // 添加到一行
            model->appendRow(row);
        }
    }
}

void MainWindow::onSocketErrorOccurred(QBluetoothSocket::SocketError error)
{
    QMessageBox::critical(this, "Socket Error", "Socket error occurred: " + QString::number(error));
}

void MainWindow::onBleErrorOccurred(QLowEnergyController::Error error)
{
    QMessageBox::critical(this, "BLE Error", "BLE error occurred: " + QString::number(error));
}
