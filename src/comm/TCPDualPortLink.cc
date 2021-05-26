/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include <QTimer>
#include <QList>
#include <QDebug>
#include <QMutexLocker>
#include <iostream>
#include "TCPDualPortLink.h"
#include "LinkManager.h"
#include "QGC.h"
#include <QHostInfo>
#include <QSignalSpy>

TCPDualPortLink::TCPDualPortLink(SharedLinkConfigurationPtr& config)
    : LinkInterface(config)
    , _tcpDualPortConfig(qobject_cast<TCPDualPortConfiguration*>(config.get()))
	, _socketTo(nullptr)
	, _socketFrom(nullptr)
    , _socketIsConnected(false)
{
    Q_ASSERT(_tcpDualPortConfig);
}

TCPDualPortLink::~TCPDualPortLink()
{
    disconnect();
}

#ifdef TCPDualPortLink_READWRITE_DEBUG
void TCPDualPortLink::_writeDebugBytes(const QByteArray data)
{
    QString bytes;
    QString ascii;
    for (int i=0, size = data.size(); i<size; i++)
    {
        unsigned char v = data[i];
        bytes.append(QString::asprintf("%02x ", v));
        if (data[i] > 31 && data[i] < 127)
        {
            ascii.append(data[i]);
        }
        else
        {
            ascii.append(219);
        }
    }
    qDebug() << "Sent" << size << "bytes to" << _tcpConfig->address().toString() << ":" << _tcpConfig->_portTo() << "data:";
    qDebug() << bytes;
    qDebug() << "ASCII:" << ascii;
}
#endif

void TCPDualPortLink::_writeBytes(const QByteArray data)
{
#ifdef TCPDualPortLink_READWRITE_DEBUG
    _writeDebugBytes(data);
#endif

    if (_socketTo) {
    	_socketTo->write(data);
        emit bytesSent(this, data);
    }
}

void TCPDualPortLink::_readBytes()
{
    if (_socketFrom) {
        qint64 byteCount = _socketFrom->bytesAvailable();
        if (byteCount)
        {
            QByteArray buffer;
            buffer.resize(byteCount);
            _socketFrom->read(buffer.data(), buffer.size());
            emit bytesReceived(this, buffer);
#ifdef TCPDualPortLink_READWRITE_DEBUG
            writeDebugBytes(buffer.data(), buffer.size());
#endif
        }
    }
}

void TCPDualPortLink::disconnect(void)
{
    if (_socketFrom) {
        // This prevents stale signal from calling the link after it has been deleted
        QObject::disconnect(_socketFrom, &QIODevice::readyRead, this, &TCPDualPortLink::_readBytes);
        _socketFrom->disconnectFromHost(); // Disconnect tcp
        _socketFrom->deleteLater(); // Make sure delete happens on correct thread
        _socketFrom = nullptr;
    } // if (_socketFrom) {

    if (_socketTo) {
        // This prevents stale signal from calling the link after it has been deleted
        QObject::disconnect(_socketTo, &QIODevice::readyRead, this, &TCPDualPortLink::_readBytes);
        _socketTo->disconnectFromHost(); // Disconnect tcp
        _socketTo->deleteLater(); // Make sure delete happens on correct thread
        _socketTo = nullptr;
    } // if (_socketTo) {

    _socketIsConnected = false;
    emit disconnected();
}



bool TCPDualPortLink::_connect(void)
{
    if (_socketIsConnected == true) {
        qWarning() << "connect called while already connected";
        return true;
    }

    return _hardwareConnect();
}



bool TCPDualPortLink::_hardwareConnect()
{
    Q_ASSERT(_socketTo == nullptr);
    Q_ASSERT(_socketFrom == nullptr);

    _socketTo = new QTcpSocket();
    QObject::connect(_socketTo, nullptr, this, nullptr);

#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
    QSignalSpy errorSpyTo(_socketTo, static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(&QTcpSocket::error));
#else
    QSignalSpy errorSpyTo(_socketTo, &QAbstractSocket::errorOccurred);
#endif
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
    QObject::connect(_socketTo,static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(&QTcpSocket::error),
                     this, &TCPDualPortLink::_socketError);
#else
    QObject::connect(_socketTo, &QAbstractSocket::errorOccurred, this, &TCPDualPortLink::_socketError);
#endif

    _socketTo->connectToHost(_tcpDualPortConfig->address(), _tcpDualPortConfig->portTo());

    // Give the socket a second to connect to the other side otherwise error out
    if (!_socketTo->waitForConnected(1000))
    {
        // Whether a failed connection emits an error signal or not is platform specific.
        // So in cases where it is not emitted, we emit one ourselves.
        if (errorSpyTo.count() == 0) {
            emit communicationError(tr("Link Error"), tr("Error on link %1. Connection failed").arg(_config->name()));
        }

        delete _socketTo;
        _socketTo = nullptr;

        return false;
    }


    _socketFrom = new QTcpSocket();
    QObject::connect(_socketFrom, &QIODevice::readyRead, this, &TCPDualPortLink::_readBytes);

#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
    QSignalSpy errorSpyFrom(_socketFrom, static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(&QTcpSocket::error));
#else
    QSignalSpy errorSpyFrom(_socketFrom, &QAbstractSocket::errorOccurred);
#endif
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
    QObject::connect(_socketFrom, static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(&QTcpSocket::error),
                     this, &TCPDualPortLink::_socketError);
#else
    QObject::connect(_socketFrom, &QAbstractSocket::errorOccurred, this, &TCPDualPortLink::_socketError);
#endif

    _socketFrom->connectToHost(_tcpDualPortConfig->address(), _tcpDualPortConfig->portFrom());

    // Give the socket a second to connect to the other side otherwise error out
    if (!_socketFrom->waitForConnected(1000))
    {
        // Whether a failed connection emits an error signal or not is platform specific.
        // So in cases where it is not emitted, we emit one ourselves.
        if (errorSpyFrom.count() == 0) {
            emit communicationError(tr("Link Error"), tr("Error on link %1. Connection failed").arg(_config->name()));
        }

        delete _socketFrom;
        _socketFrom = nullptr;


        QObject::disconnect(_socketTo, &QIODevice::readyRead, this, &TCPDualPortLink::_readBytes);
        _socketTo->disconnectFromHost(); // Disconnect tcp
        _socketTo->deleteLater(); // Make sure delete happens on correct thread
        _socketTo = nullptr;

        return false;
    }


    _socketIsConnected = true;
    emit connected();
    return true;
}



void TCPDualPortLink::_socketError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError);
    emit communicationError(tr("Link Error"), tr("Error on link %1. Error on socket: %2.").arg(_config->name()).arg(_socketTo->errorString()));
    emit communicationError(tr("Link Error"), tr("Error on link %1. Error on socket: %2.").arg(_config->name()).arg(_socketFrom->errorString()));
}



/**
 * @brief Check if connection is active.
 *
 * @return True if link is connected, false otherwise.
 **/
bool TCPDualPortLink::isConnected() const
{
    return _socketIsConnected;
}



//--------------------------------------------------------------------------
//-- TCPDualPortConfiguration

static bool is_ip(const QString& address)
{
    int a,b,c,d;
    if (sscanf(address.toStdString().c_str(), "%d.%d.%d.%d", &a, &b, &c, &d) != 4
            && strcmp("::1", address.toStdString().c_str())) {
        return false;
    } else {
        return true;
    }
}



static QString get_ip_address(const QString& address)
{
    if(is_ip(address))
        return address;
    // Need to look it up
    QHostInfo info = QHostInfo::fromName(address);
    if (info.error() == QHostInfo::NoError)
    {
        QList<QHostAddress> hostAddresses = info.addresses();
        for (int i = 0; i < hostAddresses.size(); i++)
        {
            // Exclude all IPv6 addresses
            if (!hostAddresses.at(i).toString().contains(":"))
            {
                return hostAddresses.at(i).toString();
            }
        }
    }
    return {};
}



TCPDualPortConfiguration::TCPDualPortConfiguration(const QString& name) : LinkConfiguration(name)
{
	_portTo    = QGC_TCP_PORT_TO;
	_portFrom    = QGC_TCP_PORT_FROM;
    _address = QHostAddress::Any;
}



TCPDualPortConfiguration::TCPDualPortConfiguration(TCPDualPortConfiguration* source) : LinkConfiguration(source)
{
	_portTo    = source->portTo();
	_portFrom    = source->portFrom();
    _address = source->address();
}



void TCPDualPortConfiguration::copyFrom(LinkConfiguration *source)
{
    LinkConfiguration::copyFrom(source);
    auto* usource = qobject_cast<TCPDualPortConfiguration*>(source);
    Q_ASSERT(usource != nullptr);
	_portTo    = usource->portTo();
	_portFrom    = usource->portFrom();
    _address = usource->address();
}

void TCPDualPortConfiguration::setPortTo(quint16 port)
{
	_portTo = port;
}

void TCPDualPortConfiguration::setPortFrom(quint16 port)
{
	_portFrom = port;
}

void TCPDualPortConfiguration::setAddress(const QHostAddress& address)
{
    _address = address;
}

void TCPDualPortConfiguration::setHost(const QString host)
{
    QString ipAdd = get_ip_address(host);
    if(ipAdd.isEmpty()) {
        qWarning() << "TCP:" << "Could not resolve host:" << host;
    } else {
        _address = QHostAddress(ipAdd);
    }
}

void TCPDualPortConfiguration::saveSettings(QSettings& settings, const QString& root)
{
    settings.beginGroup(root);
    settings.setValue("portTo", (int)_portTo);
    settings.setValue("portFrom", (int)_portFrom);
    settings.setValue("host", address().toString());
    settings.endGroup();
}

void TCPDualPortConfiguration::loadSettings(QSettings& settings, const QString& root)
{
    settings.beginGroup(root);
    _portTo = (quint16)settings.value("portTo", QGC_TCP_PORT_TO).toUInt();
    _portFrom = (quint16)settings.value("portFrom", QGC_TCP_PORT_FROM).toUInt();
    QString address = settings.value("host", _address.toString()).toString();
    _address = QHostAddress(address);
    settings.endGroup();
}
