#include "networkmanager.h"

#include <QTcpServer>
#include <QTcpSocket>
#include <QAbstractSocket>
#include <QDebug>
#include <QJsonDocument>
#include <QHostAddress>
#include <QTimer>
#include <QUdpSocket>
#include <QtGlobal>

NetworkManager::NetworkManager(QObject *parent)
    : QObject(parent),
      server(new QTcpServer(this)),
      socket(nullptr),
      discoverySocket(nullptr),
      discoveryTimer(new QTimer(this)),
      advertisedPort(0),
      currentMode(OfflineMode),
      currentStatus("Offline")
{
    connect(server, &QTcpServer::newConnection, this, &NetworkManager::handleNewConnection);
    connect(discoveryTimer, &QTimer::timeout, this, &NetworkManager::broadcastDiscovery);
}

NetworkManager::~NetworkManager()
{
    disconnectFromNetwork();
}

bool NetworkManager::startServer(quint16 port)
{
    disconnectFromNetwork();

    currentMode = ServerMode;

    if (!server->listen(QHostAddress::Any, port))
    {
        QString error = QString("Could not host on port %1: %2").arg(port).arg(server->errorString());
        setStatus(error);
        emit errorOccurred(error);
        currentMode = OfflineMode;
        return false;
    }

    setStatus(QString("Hosting on port %1. Waiting for Player 2...").arg(server->serverPort()));
    advertisedPort = server->serverPort();
    startAdvertising();
    qDebug() << "Server started on port" << server->serverPort();
    return true;
}

void NetworkManager::connectToServer(const QString &hostAddress, quint16 port)
{
    disconnectFromNetwork();

    currentMode = ClientMode;
    attachSocket(new QTcpSocket(this));
    setStatus(QString("Connecting to %1:%2...").arg(hostAddress).arg(port));
    qDebug() << "Connecting to host" << hostAddress << port;
    socket->connectToHost(hostAddress, port);
}

void NetworkManager::disconnectFromNetwork()
{
    if (socket)
    {
        socket->disconnect(this);
        socket->disconnectFromHost();
        socket->deleteLater();
        socket = nullptr;
    }

    if (server->isListening())
    {
        server->close();
    }

    stopAdvertising();
    stopDiscovery();
    advertisedPort = 0;
    currentMode = OfflineMode;
    setStatus("Offline");
}

void NetworkManager::startDiscovery()
{
    stopDiscovery();
    discoveredHostList.clear();

    discoverySocket = new QUdpSocket(this);

    bool bound = discoverySocket->bind(QHostAddress::AnyIPv4,
                                       DiscoveryPort,
                                       QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);

    if (!bound)
    {
        QString error = QString("Could not start host discovery on port %1: %2")
                        .arg(DiscoveryPort)
                        .arg(discoverySocket->errorString());
        discoverySocket->deleteLater();
        discoverySocket = nullptr;
        setStatus(error);
        emit errorOccurred(error);
        return;
    }

    connect(discoverySocket, &QUdpSocket::readyRead, this, &NetworkManager::handleDiscoveryDatagrams);
    setStatus(QString("Discovering multiplayer hosts on this network..."));
}

void NetworkManager::stopDiscovery()
{
    if (discoverySocket && currentMode != ServerMode)
    {
        discoverySocket->disconnect(this);
        discoverySocket->close();
        discoverySocket->deleteLater();
        discoverySocket = nullptr;
    }
}

QList<NetworkManager::DiscoveredHost> NetworkManager::discoveredHosts() const
{
    return discoveredHostList;
}

void NetworkManager::sendMessage(const QJsonObject &message)
{
    if (!socket || socket->state() != QAbstractSocket::ConnectedState)
        return;

    QByteArray data = QJsonDocument(message).toJson(QJsonDocument::Compact);
    data.append('\n');
    socket->write(data);
    socket->flush();
    qDebug() << "Message sent:" << message["type"].toString();
}

void NetworkManager::broadcastMessage(const QJsonObject &message)
{
    sendMessage(message);
}

NetworkManager::Mode NetworkManager::mode() const
{
    return currentMode;
}

bool NetworkManager::isConnected() const
{
    return socket && socket->state() == QAbstractSocket::ConnectedState;
}

QString NetworkManager::statusText() const
{
    return currentStatus;
}

void NetworkManager::handleNewConnection()
{
    QTcpSocket *incomingSocket = server->nextPendingConnection();

    if (!incomingSocket)
        return;

    if (socket)
    {
        QJsonObject fullMessage;
        fullMessage["type"] = "gameOver";
        fullMessage["reason"] = "A game is already in progress on this host.";
        QByteArray data = QJsonDocument(fullMessage).toJson(QJsonDocument::Compact);
        data.append('\n');
        incomingSocket->write(data);
        incomingSocket->disconnectFromHost();
        incomingSocket->deleteLater();
        return;
    }

    attachSocket(incomingSocket);
    setStatus(QString("Player 2 connected from %1").arg(socket->peerAddress().toString()));
    qDebug() << "Client connected from" << socket->peerAddress().toString();
    emit connected();
}

void NetworkManager::handleReadyRead()
{
    if (!socket)
        return;

    while (socket->canReadLine())
    {
        QByteArray line = socket->readLine().trimmed();
        if (line.isEmpty())
            continue;

        QJsonParseError parseError;
        QJsonDocument document = QJsonDocument::fromJson(line, &parseError);

        if (parseError.error != QJsonParseError::NoError || !document.isObject())
        {
            qDebug() << "Invalid message:" << line;
            emit errorOccurred(QString("Bad network packet: %1").arg(parseError.errorString()));
            continue;
        }

        qDebug() << "Message received:" << document.object()["type"].toString();
        emit messageReceived(document.object());
    }
}

void NetworkManager::handleDisconnected()
{
    if (currentMode == ServerMode && server->isListening())
    {
        if (socket)
        {
            socket->deleteLater();
            socket = nullptr;
        }

        setStatus("Player 2 disconnected. Waiting for a new connection...");
        qDebug() << "Client disconnected";
        emit disconnected();
        return;
    }

    if (socket)
    {
        socket->deleteLater();
        socket = nullptr;
    }

    setStatus("Disconnected");
    qDebug() << "Disconnected from network";
    emit disconnected();
}

void NetworkManager::handleSocketError()
{
    if (!socket)
        return;

    QString error = socket->errorString();
    setStatus(error);
    qDebug() << "Connection failed:" << error;
    emit errorOccurred(error);
}

void NetworkManager::broadcastDiscovery()
{
    if (currentMode != ServerMode || !server->isListening())
        return;

    if (!discoverySocket)
        return;

    QJsonObject message;
    message["type"] = "FishyHosts";
    message["name"] = "Sovereign Bros Host";
    message["port"] = static_cast<int>(advertisedPort);

    QByteArray data = QJsonDocument(message).toJson(QJsonDocument::Compact);
    discoverySocket->writeDatagram(data, QHostAddress::Broadcast, DiscoveryPort);
}

void NetworkManager::handleDiscoveryDatagrams()
{
    if (!discoverySocket)
        return;

    while (discoverySocket->hasPendingDatagrams())
    {
        QByteArray datagram;
        datagram.resize(static_cast<int>(discoverySocket->pendingDatagramSize()));
        QHostAddress senderAddress;
        quint16 senderPort = 0;
        discoverySocket->readDatagram(datagram.data(), datagram.size(), &senderAddress, &senderPort);
        Q_UNUSED(senderPort);

        QJsonParseError parseError;
        QJsonDocument document = QJsonDocument::fromJson(datagram, &parseError);

        if (parseError.error != QJsonParseError::NoError || !document.isObject())
            continue;

        QJsonObject message = document.object();
        if (message["type"].toString() != "FishyHosts")
            continue;

        quint16 port = static_cast<quint16>(message["port"].toInt());
        if (port == 0)
            continue;

        QString address = senderAddress.toString();
        if (address.startsWith("::ffff:"))
        {
            address = address.mid(7);
        }

        bool knownHost = false;
        for (const DiscoveredHost &host : discoveredHostList)
        {
            if (host.address == address && host.port == port)
            {
                knownHost = true;
                break;
            }
        }

        if (knownHost)
            continue;

        DiscoveredHost host;
        host.name = message["name"].toString("Sovereign Bros Host");
        host.address = address;
        host.port = port;
        discoveredHostList.append(host);
        setStatus(QString("Found host %1:%2").arg(host.address).arg(host.port));
    }
}

void NetworkManager::attachSocket(QTcpSocket *newSocket)
{
    socket = newSocket;
    connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::handleReadyRead);
    connect(socket, &QTcpSocket::disconnected, this, &NetworkManager::handleDisconnected);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(socket, &QAbstractSocket::errorOccurred, this, &NetworkManager::handleSocketError);
#else
    connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(handleSocketError()));
#endif
    connect(socket, &QTcpSocket::connected, this, [this]()
    {
        setStatus("Connected");
        qDebug() << "Connected";
        emit connected();
    });
}

void NetworkManager::startAdvertising()
{
    if (!discoverySocket)
    {
        discoverySocket = new QUdpSocket(this);
    }

    discoveryTimer->start(1000);
    broadcastDiscovery();
}

void NetworkManager::stopAdvertising()
{
    discoveryTimer->stop();

    if (discoverySocket && currentMode == ServerMode)
    {
        discoverySocket->close();
        discoverySocket->deleteLater();
        discoverySocket = nullptr;
    }
}

void NetworkManager::setStatus(const QString &status)
{
    currentStatus = status;
    emit statusChanged(currentStatus);
}
