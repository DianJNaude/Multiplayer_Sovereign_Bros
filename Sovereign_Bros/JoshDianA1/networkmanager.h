#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QObject>
#include <QJsonObject>
#include <QList>

class QTcpServer;
class QTcpSocket;
class QTimer;
class QUdpSocket;

class NetworkManager : public QObject
{
    Q_OBJECT

public:
    struct DiscoveredHost
    {
        QString name;
        QString address;
        quint16 port;
    };

    enum Mode
    {
        OfflineMode,
        ServerMode,
        ClientMode
    };

    static const quint16 DiscoveryPort = 45455;

    explicit NetworkManager(QObject *parent = nullptr);
    ~NetworkManager();

    bool startServer(quint16 port);
    void connectToServer(const QString &hostAddress, quint16 port);
    void disconnectFromNetwork();
    void startDiscovery();
    void stopDiscovery();
    QList<DiscoveredHost> discoveredHosts() const;

    void sendMessage(const QJsonObject &message);
    void broadcastMessage(const QJsonObject &message);

    Mode mode() const;
    bool isConnected() const;
    QString statusText() const;

signals:
    void connected();
    void disconnected();
    void messageReceived(const QJsonObject &message);
    void statusChanged(const QString &status);
    void errorOccurred(const QString &error);

private slots:
    void handleNewConnection();
    void handleReadyRead();
    void handleDisconnected();
    void handleSocketError();
    void broadcastDiscovery();
    void handleDiscoveryDatagrams();

private:
    void attachSocket(QTcpSocket *newSocket);
    void startAdvertising();
    void stopAdvertising();
    void setStatus(const QString &status);

private:
    QTcpServer *server;
    QTcpSocket *socket;
    QUdpSocket *discoverySocket;
    QTimer *discoveryTimer;
    QList<DiscoveredHost> discoveredHostList;
    quint16 advertisedPort;
    Mode currentMode;
    QString currentStatus;
};

#endif
