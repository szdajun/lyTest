#include "BlueDevice.h"
#include <QDebug>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothSocket>
#include <QBluetoothServiceDiscoveryAgent>
#include <QLowEnergyController>
#include <QLowEnergyService>
#include <QBluetoothUuid>
#include <QLowEnergyCharacteristic>  // 包含此头文件
#include <QLowEnergyCharacteristicData>
#include <QLowEnergyDescriptor>

BlueDevice::BlueDevice(QObject *parent)
    : QObject(parent)
{
    qDebug() << "初始化蓝牙设备...";

    // 创建蓝牙设备扫描对象
    discoveryAgent.reset(new QBluetoothDeviceDiscoveryAgent(this));
    connect(discoveryAgent.data(), &QBluetoothDeviceDiscoveryAgent::deviceDiscovered,
            this, &BlueDevice::deviceDiscoveredSlot);
    connect(discoveryAgent.data(), &QBluetoothDeviceDiscoveryAgent::finished,
            this, &BlueDevice::discoveryFinished);

    // 创建 RFCOMM 经典蓝牙套接字
    socket.reset(new QBluetoothSocket(QBluetoothServiceInfo::RfcommProtocol, this));
    connect(socket.data(), &QBluetoothSocket::connected, this, &BlueDevice::socketConnected);
    connect(socket.data(), &QBluetoothSocket::disconnected, this, &BlueDevice::socketDisconnected);
    connect(socket.data(), &QBluetoothSocket::readyRead, this, &BlueDevice::readSocketData);
    connect(socket.data(), QOverload<QBluetoothSocket::SocketError>::of(&QBluetoothSocket::error),
            this, &BlueDevice::handleSocketError);
}

BlueDevice::~BlueDevice()
{
    qDebug() << "销毁蓝牙设备...";
}

void BlueDevice::startDiscovery()
{
    qDebug() << "开始搜索蓝牙设备...";
    discoveryAgent->start();
}

void BlueDevice::deviceDiscoveredSlot(const QBluetoothDeviceInfo &device)
{
    qDebug() << "发现设备: " << device.name() << " 地址: " << device.address().toString();
    if (device.coreConfigurations() & QBluetoothDeviceInfo::LowEnergyCoreConfiguration) {
        qDebug() << "这是一个BLE 设备";
    } else {
        qDebug() << "这是一个经典蓝牙 (RFCOMM) 设备";
        // 启动经典蓝牙服务发现
        if (!serviceDiscoveryAgent) {
            serviceDiscoveryAgent.reset(new QBluetoothServiceDiscoveryAgent(device.address(), this));
            connect(serviceDiscoveryAgent.data(), &QBluetoothServiceDiscoveryAgent::serviceDiscovered,
                    this, &BlueDevice::serviceDiscoveredClassic);
            connect(serviceDiscoveryAgent.data(), &QBluetoothServiceDiscoveryAgent::finished,
                    this, &BlueDevice::discoveryFinished);
        }
        serviceDiscoveryAgent->start();
    }
    emit deviceDiscovered(device);
}

void BlueDevice::discoveryFinished()
{
    qDebug() << "蓝牙设备搜索完成";
}

void BlueDevice::connectToDevice(const QBluetoothDeviceInfo &device)
{
    if (device.coreConfigurations() & QBluetoothDeviceInfo::LowEnergyCoreConfiguration) {
        // 连接 BLE 设备
        isLowEnergyDevice = true;
        bleController.reset(QLowEnergyController::createCentral(device, this));
        connect(bleController.data(), &QLowEnergyController::connected, this, &BlueDevice::bleConnected);
        connect(bleController.data(), &QLowEnergyController::disconnected, this, &BlueDevice::bleDisconnected);
        connect(bleController.data(), &QLowEnergyController::serviceDiscovered, this, &BlueDevice::serviceDiscovered);
        connect(bleController.data(), QOverload<QLowEnergyController::Error>::of(&QLowEnergyController::error),
                this, &BlueDevice::bleError); // 连接错误处理
        bleController->connectToDevice();
    } else {
        // 连接经典蓝牙设备
        isLowEnergyDevice = false;
        if (socket->state() != QBluetoothSocket::ConnectedState) {
            socket->connectToService(device.address(), QBluetoothUuid(QBluetoothUuid::SerialPort));
        }
    }
}

void BlueDevice::serviceDiscoveredClassic(const QBluetoothServiceInfo &service)
{
    qDebug() << "发现经典蓝牙服务: " << service.serviceName() << " 地址: " << service.device().address().toString();
    // 选择合适的服务进行连接（例如 SerialPort）
    if (service.serviceUuid() == QBluetoothUuid(QBluetoothUuid::SerialPort)) {
        if (socket->state() != QBluetoothSocket::ConnectedState) {
            socket->connectToService(service.device().address(), service.serviceUuid());
        }
    }
}

void BlueDevice::socketConnected()
{
    qDebug() << "经典蓝牙 (RFCOMM) 连接成功!";
    emit connectionEstablished();
}

void BlueDevice::socketDisconnected()
{
    qDebug() << "RFCOMM 连接已断开";
    emit connectionLost();
}

void BlueDevice::sendData(const QByteArray &data)
{
    if (isLowEnergyDevice) {
        if (bleService) {
            QLowEnergyCharacteristic charac = bleService->characteristic(QBluetoothUuid(quint16(0xFFF6)));
            if (charac.isValid()) {
                bleService->writeCharacteristic(charac, data, QLowEnergyService::WriteWithResponse); // 使用 WriteWithResponse
                qDebug() << "发送 BLE 数据: " << data;
            } else {
                qDebug() << "无效的 BLE 特征!";
            }
        } else {
            qDebug() << "BLE 服务未找到!";
        }
    } else {
        if (socket->isOpen()) {
            socket->write(data);
            qDebug() << "发送 RFCOMM 数据: " << data;
        } else {
            qDebug() << "RFCOMM 套接字未打开!";
        }
    }
}

void BlueDevice::readSocketData()
{
    QByteArray receivedData = socket->readAll();
    qDebug() << "收到 RFCOMM 数据: " << receivedData;
    emit dataReceived(receivedData); // 将接收到的数据通过信号发送
}

void BlueDevice::bleConnected()
{
    qDebug() << "BLE 设备连接成功!";
    bleController->discoverServices();
}

void BlueDevice::bleDisconnected()
{
    qDebug() << "BLE 设备断开";
    emit connectionLost();
}

void BlueDevice::serviceDiscovered(const QBluetoothUuid &uuid)
{
    qDebug() << "发现 BLE 服务: " << uuid;
    // 修改：判断发现的服务 UUID 是否符合你的需求
    if (uuid == QBluetoothUuid(quint16(0xFFF0))) { // 假设 0xFFF0 是你的服务 UUID
        bleService.reset(bleController->createServiceObject(uuid, this));
        if (bleService) {
            connect(bleService.data(), &QLowEnergyService::stateChanged,
                    this, &BlueDevice::bleServiceStateChanged); // 监听服务状态变化
            connect(bleService.data(), &QLowEnergyService::characteristicChanged, this, &BlueDevice::characteristicChanged);
            connect(bleService.data(), &QLowEnergyService::characteristicWritten, this, &BlueDevice::characteristicWritten); // 添加写完成信号的连接
            bleService->discoverDetails();
        } else {
            qDebug() << "无法创建 BLE 服务对象!";
        }
    }
}

void BlueDevice::characteristicChanged(const QLowEnergyCharacteristic &c, const QByteArray &value)
{
    Q_UNUSED(c);
    qDebug() << "收到 BLE 数据: " << value;
    emit dataReceived(value); // 发送数据
}

// 添加写完成槽函数
void BlueDevice::characteristicWritten(const QLowEnergyCharacteristic &characteristic, const QByteArray &value)
{
    Q_UNUSED(characteristic)
    Q_UNUSED(value)
    qDebug() << "BLE 数据写入完成";
}

// 添加服务状态变化槽函数
void BlueDevice::bleServiceStateChanged(QLowEnergyService::ServiceState s)
{
    qDebug() << "BLE 服务状态变化: " << s;
    if (s == QLowEnergyService::ServiceDiscovered) {
        // 发现服务详细信息后，查找特征值并启用通知
        QLowEnergyCharacteristic charac = bleService->characteristic(QBluetoothUuid(quint16(0xFFF6)));
        if (charac.isValid()) {
            // QLowEnergyCharacteristicData charData = charac.properties(); // 注释掉这行
            QLowEnergyCharacteristic::PropertyTypes charData = charac.properties(); // 使用正确的类型
            if (charData & QLowEnergyCharacteristic::PropertyType::Notify) { // 使用位运算判断是否支持 Notify
                // 启用通知
                QLowEnergyDescriptor desc = charac.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration);
                if (desc.isValid()) {
                    bleService->writeDescriptor(desc, QByteArray::fromHex("0100")); // 启用 Notification
                    qDebug() << "启用 BLE 通知";
                } else {
                    qDebug() << "找不到 CCC 描述符!";
                }
            } else {
                qDebug() << "该特征值不支持 Notify!";
            }
        } else {
            qDebug() << "找不到特征值!";
        }
    }
}


void BlueDevice::disconnectDevice()
{
    if (isLowEnergyDevice && bleController) {
        bleController->disconnectFromDevice();
    } else if (!isLowEnergyDevice && socket->isOpen()) {
        socket->close();
    }
}

void BlueDevice::handleSocketError(QBluetoothSocket::SocketError error)
{
    qDebug() << "Socket error: " << error;
    switch (error) {
    case QBluetoothSocket::HostNotFoundError:
        //qDebug() << QString::fromLocal8Bit("主机未找到");
        break;
    case QBluetoothSocket::ServiceNotFoundError:
        //qDebug() << "服务未找到";
        break;
    case QBluetoothSocket::NoSocketError:
        //qDebug() << "网络错误";
        break;
    case QBluetoothSocket::UnsupportedProtocolError:
        //qDebug() << "网络错误";
        break;
    case QBluetoothSocket::OperationError:
        //qDebug() << "网络错误";
        break;
    case QBluetoothSocket::RemoteHostClosedError:
        break;
    case QBluetoothSocket::NetworkError:
        //qDebug() << "网络错误";
        break;
    case QBluetoothSocket::UnknownSocketError:
    default:
        //qDebug() << "其他错误";
        break;
    }
    emit socketErrorOccurred(error); // 发送错误信号给主窗口
}

void BlueDevice::bleError(QLowEnergyController::Error error)
{
    qDebug() << "BLE Controller error: " << error;
    emit bleErrorOccurred(error); // 发送 BLE 错误信号
}

