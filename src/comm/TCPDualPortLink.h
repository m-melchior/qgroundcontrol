/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QString>
#include <QList>
#include <QMap>
#include <QMutex>
#include <QHostAddress>
#include <LinkInterface.h>
#include "QGCConfig.h"

// Even though QAbstractSocket::SocketError is used in a signal by Qt, Qt doesn't declare it as a meta type.
// This in turn causes debug output to be kicked out about not being able to queue the signal. We declare it
// as a meta type to silence that.
#include <QMetaType>
#include <QTcpSocket>

//#define TCPLINK_READWRITE_DEBUG   // Use to debug data reads/writes

class TCPDualPortLinkTest;
class LinkManager;

#define QGC_TCP_PORT_TO 8888
#define QGC_TCP_PORT_FROM 8080

class TCPDualPortConfiguration : public LinkConfiguration
{
    Q_OBJECT

public:

    Q_PROPERTY(quint16 portTo READ portTo WRITE setPortTo NOTIFY portToChanged)
    Q_PROPERTY(quint16 portFrom READ portFrom WRITE setPortFrom NOTIFY portFromChanged)
    Q_PROPERTY(QString host READ host WRITE setHost NOTIFY hostChanged)

    TCPDualPortConfiguration(const QString& name);
    TCPDualPortConfiguration(TCPDualPortConfiguration* source);

    quint16             portTo     (void) const                    { return _portTo; }
    quint16             portFrom   (void) const                    { return _portFrom; }
    const QHostAddress& address     (void)                          { return _address; }
    const QString       host        (void)                          { return _address.toString(); }
    void                setPortTo   (quint16 port);
    void                setPortFrom (quint16 port);
    void                setAddress  (const QHostAddress& address);
    void                setHost     (const QString host);

    //LinkConfiguration overrides
    LinkType    type                (void) override                                         { return LinkConfiguration::TypeTcpDualPort; }
    void        copyFrom            (LinkConfiguration* source) override;
    void        loadSettings        (QSettings& settings, const QString& root) override;
    void        saveSettings        (QSettings& settings, const QString& root) override;
    QString     settingsURL         (void) override                                         { return "TcpDualPortSettings.qml"; }
    QString     settingsTitle       (void) override                                         { return tr("TCP Dual Port Link Settings"); }

signals:
	void portToChanged(void);
	void portFromChanged(void);
    void hostChanged(void);

private:
    QHostAddress    _address;
    quint16         _portTo;
    quint16         _portFrom;
};

class TCPDualPortLink : public LinkInterface
{
    Q_OBJECT

public:
	TCPDualPortLink(SharedLinkConfigurationPtr& config);
    virtual ~TCPDualPortLink();

    QTcpSocket* getSocketTo         (void) { return _socketTo; }
    QTcpSocket* getSocketFrom       (void) { return _socketFrom; }
    void        signalBytesWritten  (void);

    // LinkInterface overrides
    bool isConnected(void) const override;
    void disconnect (void) override;

private slots:
    void _socketError   (QAbstractSocket::SocketError socketError);
    void _readBytes     (void);

    // LinkInterface overrides
    void _writeBytes(const QByteArray data) override;

private:
    // LinkInterface overrides
    bool _connect(void) override;

    bool _hardwareConnect   (void);
#ifdef TCPDUALPORTLINK_READWRITE_DEBUG
    void _writeDebugBytes   (const QByteArray data);
#endif

    TCPDualPortConfiguration* _tcpDualPortConfig;
    QTcpSocket*       _socketTo;
    QTcpSocket*       _socketFrom;
    bool              _socketIsConnected;

    quint64 _bitsSentTotal;
    quint64 _bitsSentCurrent;
    quint64 _bitsSentMax;
    quint64 _bitsReceivedTotal;
    quint64 _bitsReceivedCurrent;
    quint64 _bitsReceivedMax;
    quint64 _connectionStartTime;
    QMutex  _statisticsMutex;
};

