#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStandardItemModel>
#include <QBluetoothDeviceInfo>
#include <QBluetoothServiceInfo>

#include "bluedevice.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onStartDiscoveryClicked(); // 开始设备发现
    void onStopDiscoveryClicked(); // 停止设备发现
    void onDeviceSelected(const QModelIndex &index); // 选择设备进行连接
    void onServiceSelected(int index); // 选择服务进行连接
    void onSendDataClicked(); // 发送数据
    void onDataReceived(const QByteArray &data); // 接收到数据

    void onConnectionEstablished(); // 连接成功
    void onConnectionLost(); // 连接断开

    void onDisconnectButtonClicked(); // 断开连接

    void onDeviceDiscovered(const QBluetoothDeviceInfo &device); // 发现设备
    void onSocketErrorOccurred(QBluetoothSocket::SocketError error); // 显示套接字错误
    void onBleErrorOccurred(QLowEnergyController::Error error); // 显示BLE错误

private:
    Ui::MainWindow *ui;
    BlueDevice *blueDevice; // 蓝牙设备处理对象
    QBluetoothDeviceInfo selectedDevice; // 当前选中的设备
    QList<QBluetoothServiceInfo> availableServices; // 设备提供的服务列表
};
#endif // MAINWINDOW_H
