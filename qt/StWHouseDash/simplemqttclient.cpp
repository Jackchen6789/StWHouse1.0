#include "simplemqttclient.h"
#include <QNetworkProxy>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlQuery>
#include <QSqlDatabase>
#include <QSqlError>

SimpleMqttClient::SimpleMqttClient(QObject *parent)
    : QObject(parent), m_socket(nullptr), m_connected(false), m_packetId(1)
{
    m_socket = new QTcpSocket(this);
    m_socket->setProxy(QNetworkProxy::NoProxy);

    connect(m_socket, &QTcpSocket::readyRead,    this, &SimpleMqttClient::onSocketReadyRead);
    connect(m_socket, &QTcpSocket::connected,     this, &SimpleMqttClient::onSocketConnected);
    connect(m_socket, &QTcpSocket::disconnected,  this, &SimpleMqttClient::onSocketDisconnected);
    connect(m_socket, &QTcpSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) {
        qDebug() << "[MQTT] Socket error:" << m_socket->errorString();
    });

    m_pingTimer = new QTimer(this);
    connect(m_pingTimer, &QTimer::timeout, this, [this]() {
        if (m_socket && m_socket->isOpen() && m_connected) {
            m_socket->write("\xC0\x00", 2);
            qDebug() << "[MQTT] PINGREQ sent";
        }
    });

    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setSingleShot(true);
    connect(m_reconnectTimer, &QTimer::timeout, this, &SimpleMqttClient::onReconnectTimer);
}

void SimpleMqttClient::connectToBroker(const QString &host, quint16 port, const QString &username, const QString &password)
{
    m_host = host;
    m_username = username;
    m_password = password;
    m_port = port;
    m_socket->setProxy(QNetworkProxy::NoProxy);
    qDebug() << "[MQTT] Connecting to" << host << ":" << port;
    m_socket->connectToHost(host, port);
}

void SimpleMqttClient::onReconnectTimer()
{
    if (!m_connected) {
        qDebug() << "[MQTT] Reconnecting to" << m_host << ":" << m_port;
        m_socket->setProxy(QNetworkProxy::NoProxy);
        m_socket->connectToHost(m_host, m_port);
    }
}

void SimpleMqttClient::onSocketConnected()
{
    qDebug() << "[MQTT] TCP connected, sending CONNECT...";

    QByteArray pkt;
    pkt.append('\x10');

    QByteArray body;
    body.append("\x00\x04MQTT", 6);
    body.append('\x04');
    body.append('\xC2');  // Flags: Username + Password + CleanSession
    body.append("\x00\x3c", 2);
    body.append("\x00\x06QtDash", 8);

    // MQTT 认证: 用户名
    QByteArray userBytes = m_username.toUtf8();
    if (userBytes.length() > 0) {
        body.append(static_cast<char>((userBytes.length() >> 8) & 0xFF));
        body.append(static_cast<char>(userBytes.length() & 0xFF));
        body.append(userBytes);
    }

    // MQTT 认证: 密码
    QByteArray passBytes = m_password.toUtf8();
    if (passBytes.length() > 0) {
        body.append(static_cast<char>((passBytes.length() >> 8) & 0xFF));
        body.append(static_cast<char>(passBytes.length() & 0xFF));
        body.append(passBytes);
    }

    pkt.append(static_cast<char>(body.length()));
    pkt.append(body);

    m_socket->write(pkt); m_socket->flush();
    qDebug() << "[MQTT] CONNECT sent (" << pkt.length() << " bytes)";
}

void SimpleMqttClient::onSocketDisconnected()
{
    qDebug() << "[MQTT] Disconnected from broker";
    m_connected = false;
    m_pingTimer->stop();
    emit connectionStatusChanged(false);
    emit disconnected();
    // 5??????
    m_reconnectTimer->start(5000);
}

void SimpleMqttClient::subscribeToTopic(const QString &topic)
{
    qDebug() << "[MQTT] Subscribing to:" << topic;

    QByteArray pkt;
    pkt.append('\x82');

    QByteArray body;
    body.append(static_cast<char>((m_packetId >> 8) & 0xFF));
    body.append(static_cast<char>(m_packetId & 0xFF));
    m_packetId++;

    QByteArray topicBytes = topic.toUtf8();
    quint16 tlen = static_cast<quint16>(topicBytes.length());
    body.append(static_cast<char>((tlen >> 8) & 0xFF));
    body.append(static_cast<char>(tlen & 0xFF));
    body.append(topicBytes);
    body.append('\x01');

    pkt.append(static_cast<char>(body.length()));
    pkt.append(body);

    m_socket->write(pkt); m_socket->flush();
}

void SimpleMqttClient::publishMessage(const QString &topic, const QByteArray &message)
{
    if (!m_socket || !m_socket->isOpen()) {
        qDebug() << "[MQTT] Cannot publish: not connected";
        return;
    }

    QByteArray pkt;
    pkt.append('\x30');

    QByteArray body;
    QByteArray topicBytes = topic.toUtf8();
    quint16 tlen = static_cast<quint16>(topicBytes.length());
    body.append(static_cast<char>((tlen >> 8) & 0xFF));
    body.append(static_cast<char>(tlen & 0xFF));
    body.append(topicBytes);
    body.append(message);

    pkt.append(static_cast<char>(body.length()));
    pkt.append(body);

    m_socket->write(pkt); m_socket->flush();
    qDebug() << "[MQTT] PUBLISH to" << topic << ":" << message;
}

void SimpleMqttClient::onSocketReadyRead()
{
    m_buffer.append(m_socket->readAll());
    qDebug() << "[MQTT] Received" << m_buffer.size() << "bytes total in buffer";

    while (m_buffer.size() >= 2)
    {
        quint8 byte0 = static_cast<quint8>(m_buffer[0]);
        quint8 pktType = (byte0 >> 4) & 0x0F;

        int remainingLen = 0;
        int multiplier = 1;
        int pos = 1;
        while (pos < m_buffer.size() && pos < 5)
        {
            quint8 byte = static_cast<quint8>(m_buffer[pos]);
            remainingLen += (byte & 0x7F) * multiplier;
            pos++;
            if ((byte & 0x80) == 0) break;
            multiplier *= 128;
        }

        int totalLen = pos + remainingLen;
        if (m_buffer.size() < totalLen) {
            qDebug() << "[MQTT] Waiting for more data (have" << m_buffer.size() << "need" << totalLen << ")";
            return;
        }

        QByteArray packet = m_buffer.left(totalLen);
        m_buffer.remove(0, totalLen);

        switch (pktType) {
        case 2:  // CONNACK
            qDebug() << "[MQTT] CONNACK received (len=" << remainingLen << ")";
            if (remainingLen >= 2 && packet.size() >= 4) {
                quint8 retCode = static_cast<quint8>(packet[3]);
                qDebug() << "[MQTT] CONNACK return code:" << retCode << (retCode == 0 ? "(Accepted)" : "(REJECTED!)");
                if (retCode == 0) {
                    m_connected = true;
                    m_pingTimer->start(30000);
                    m_reconnectTimer->stop();
                    emit connectionStatusChanged(true);
                    emit connected();
                } else {
                    qDebug() << "[MQTT] Connection rejected, disconnecting...";
                    m_socket->disconnectFromHost();
                }
            }
            break;

        case 3: { // PUBLISH
            int topicLen = (static_cast<quint8>(packet[pos]) << 8) | static_cast<quint8>(packet[pos+1]);
            QString topic = QString::fromUtf8(packet.mid(pos+2, topicLen));
            int payloadStart = pos + 2 + topicLen;

            quint8 qos = (byte0 >> 1) & 0x03;
            if (qos > 0)
                payloadStart += 2;

            QByteArray payload = packet.mid(payloadStart);
            qDebug() << "[MQTT] PUBLISH topic:" << topic << "payload:" << payload;

            // ????: rfid ??
            qDebug() << "[RFID-CHECK] topic=[" << topic << "] len=" << topic.length() << "match=" << (topic == "wms/warehouse1/rfid");
            // RFID/卡号入库: rfid主题 或 data JSON 中的 card/uid 字段
            {
                QJsonDocument doc = QJsonDocument::fromJson(payload);
                if (!doc.isNull() && doc.isObject())
                {
                    QJsonObject obj = doc.object();
                    QString uid;
                    if (obj.contains("uid")) {
                        uid = obj["uid"].toString().trimmed();
                        qDebug() << "[RFID-DBG] uid field found:" << uid << "len=" << uid.length();
                    } else if (obj.contains("card")) {
                        uid = obj["card"].toString().trimmed();
                        qDebug() << "[RFID-DBG] card field found:" << uid << "len=" << uid.length();
                    }
                    if (!uid.isEmpty() && uid.length() >= 8)
                    {
                        uid = uid.left(8);  // 取前8位
                        QSqlQuery q(QSqlDatabase::database());
                        q.prepare("INSERT OR IGNORE INTO cards (uid) VALUES (:uid)");
                        q.bindValue(":uid", uid);
                        if (q.exec())
                            qDebug() << "[RFID] Card saved:" << uid;
                        else
                            qDebug() << "[RFID] INSERT failed:" << q.lastError().text();
                    }
                    else if (!uid.isEmpty())
                        qDebug() << "[RFID-DBG] uid too short, ignored:" << uid;
                }
            }

            emit messageReceived(topic, payload);
            break;
        }

        case 9:  // SUBACK
            qDebug() << "[MQTT] SUBACK received";
            break;

        case 13: // PINGRESP
            qDebug() << "[MQTT] PINGRESP received";
            break;

        default:
            qDebug() << "[MQTT] Unknown packet type:" << pktType << "len:" << remainingLen;
            break;
        }
    }
}
