#ifndef SIMPLEMQTTCLIENT_H
#define SIMPLEMQTTCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QTimer>

class SimpleMqttClient : public QObject
{
    Q_OBJECT
public:
    explicit SimpleMqttClient(QObject *parent = nullptr);
    void connectToBroker(const QString &host, quint16 port, const QString &username = "", const QString &password = "");
    void subscribeToTopic(const QString &topic);
    void publishMessage(const QString &topic, const QByteArray &message);
    bool isConnected() const { return m_connected; }

signals:
    void messageReceived(const QString &topic, const QByteArray &payload);
    void connected();
    void disconnected();
    void connectionStatusChanged(bool connected);

private slots:
    void onSocketReadyRead();
    void onSocketConnected();
    void onSocketDisconnected();
    void onReconnectTimer();

private:
    QTcpSocket *m_socket;
    QTimer     *m_pingTimer;
    QTimer     *m_reconnectTimer;
    QByteArray  m_buffer;
    QString     m_host;
    quint16     m_port;
    QString     m_username;
    QString     m_password;
    bool        m_connected;
    quint16     m_packetId;   // MQTT å ID è®¡æ°å¨
};

#endif // SIMPLEMQTTCLIENT_H
