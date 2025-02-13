#ifndef BLUEDEVICE_H
#define BLUEDEVICE_H

#include <QObject>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothSocket>
#include <QBluetoothServiceDiscoveryAgent>
#include <QBluetoothDeviceInfo>
#include <QBluetoothServiceInfo>
#include <QBluetoothUuid>
#include <QLowEnergyController>
#include <QLowEnergyService>
#include <QLowEnergyCharacteristic>
#include <QScopedPointer>

class BlueDevice : public QObject {
    Q_OBJECT
public:
    explicit BlueDevice(QObject *parent = nullptr);
    ~BlueDevice();

    void startDiscovery(); // 开始搜索蓝牙设备
    void connectToDevice(const QBluetoothDeviceInfo &device); // 连接设备
    void sendData(const QByteArray &data); // 发送数据
    void disconnectDevice(); // 断开连接
    QBluetoothDeviceDiscoveryAgent* getDiscoveryAgent() const { return discoveryAgent.data(); }


signals:
    void deviceDiscovered(const QBluetoothDeviceInfo &device); // 发现设备
    void connectionEstablished(); // 连接成功
    void connectionLost(); // 连接断开
    void dataReceived(const QByteArray &data); // 接收到数据
    void socketErrorOccurred(QBluetoothSocket::SocketError error); // RFCOMM 错误信号
    void bleErrorOccurred(QLowEnergyController::Error error); // BLE 错误信号

private slots:
    void deviceDiscoveredSlot(const QBluetoothDeviceInfo &device); // 发现设备
    void discoveryFinished(); // 搜索完成
    void socketConnected(); // RFCOMM 连接成功
    void socketDisconnected(); // RFCOMM 连接断开
    void readSocketData(); // 读取 RFCOMM 数据
    void handleSocketError(QBluetoothSocket::SocketError error); // 处理 RFCOMM 错误
    void bleConnected(); // BLE 连接成功
    void bleDisconnected(); // BLE 断开
    void serviceDiscovered(const QBluetoothUuid &uuid); // 发现 BLE 服务
    void characteristicChanged(const QLowEnergyCharacteristic &c, const QByteArray &value); // BLE 数据变化
    void serviceDiscoveredClassic(const QBluetoothServiceInfo &service); // 发现经典蓝牙服务
    void characteristicWritten(const QLowEnergyCharacteristic &characteristic, const QByteArray &value); // BLE 特征值写入完成
    void bleServiceStateChanged(QLowEnergyService::ServiceState s); // BLE 服务状态变化

    void bleError(QLowEnergyController::Error error); // 处理 BLE 连接错误

private:
    QScopedPointer<QBluetoothDeviceDiscoveryAgent> discoveryAgent; // 使用智能指针管理
    QScopedPointer<QBluetoothSocket> socket; // RFCOMM 连接
    QScopedPointer<QLowEnergyController> bleController; // BLE 设备控制器
    QScopedPointer<QLowEnergyService> bleService; // BLE 服务对象
    QScopedPointer<QBluetoothServiceDiscoveryAgent> serviceDiscoveryAgent; // 经典蓝牙服务发现
    bool isLowEnergyDevice {false}; // 是否是 BLE 设备
};

#endif // BLUEDEVICE_H
