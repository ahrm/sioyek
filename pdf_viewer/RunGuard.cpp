#ifndef SIOYEK_ANDROID

#include "RunGuard.h"

#include <QCryptographicHash>
#include <QtCore>
#include <QTimer>

namespace
{
    QString generateKeyHash(const QString& key, const QString& salt)
    {
        QByteArray data;
        data.append(key.toUtf8());
        data.append(salt.toUtf8());
        data = QCryptographicHash::hash(data, QCryptographicHash::Sha1).toHex();
        return data;
    }
}

RunGuard::RunGuard(const QString& key) : QObject{},
key(key),
memoryKey(generateKeyHash(key, "_sharedMemKey"))
{
    // By explicitly attaching it and then deleting it we make sure that the
    // memory is deleted even after the process has crashed on Unix.
    memory = new QSharedMemory{ memoryKey };
    memory->attach();
    delete memory;

    // Guarantee process safe behaviour with a shared memory block.
    memory = new QSharedMemory(memoryKey);

    // Creates a shared memory segment then attaches to it with the given
    // access mode and returns true.
    // If a shared memory segment identified by the key already exists, the
    // attach operation is not performed and false is returned.

    //qDebug() << "Creating shared memory block...";
    bool isPrimary = false;
    if (memory->create(sizeof(quint64))) {
        //qDebug() << "Shared memory created: this is the primary application.";
        isPrimary = true;
    }
    else {
        //qDebug() << "Shared memory already exists: this is a secondary application.";
        //qDebug() << "Secondary application attaching to shared memory block...";
        if (!memory->attach()) {
            qCritical() << "Secondary application cannot attach to shared memory block.";
            QCoreApplication::exit();
        }
        //qDebug() << "Secondary application successfully attached to shared memory block.";
    }

    memory->lock();
    if (isPrimary) { // Start primary server.
        //qDebug() << "Starting IPC server...";
        QLocalServer::removeServer(key);
        server = new QLocalServer;
        server->setSocketOptions(QLocalServer::UserAccessOption);
        if (server->listen(key)) {
            //qDebug() << "IPC server started.";
        }
        else {
            qCritical() << "Cannot start IPC server.";
            QCoreApplication::exit();
        }
        QObject::connect(server, &QLocalServer::newConnection,
            this, &RunGuard::onNewConnection);
    }
    memory->unlock();
}

RunGuard::~RunGuard()
{
    memory->lock();
    if (server) {
        server->close();
        delete server;
        server = nullptr;
    }
    memory->unlock();
}

bool RunGuard::isPrimary()
{
    return server != nullptr;
}

bool RunGuard::isSecondary()
{
    return server == nullptr;
}

void RunGuard::onNewConnection()
{
    QLocalSocket* socket = server->nextPendingConnection();
    QObject::connect(socket, &QLocalSocket::disconnected, this,
        [this, socket]() {
            if (on_delete) {
                on_delete(socket);
            }
            socket->deleteLater();
        }
    );
    QObject::connect(socket, &QLocalSocket::readyRead, this, [socket, this]() {
        readMessage(socket);
        });
}

void RunGuard::readMessage(QLocalSocket* socket)
{
    QByteArray data = socket->readAll();
    emit messageReceived(data, socket);
}

std::string RunGuard::sendMessage(const QByteArray& message, bool wait)
{
    QLocalSocket socket;
    socket.connectToServer(key, QLocalSocket::ReadWrite);
    socket.waitForConnected();
    if (socket.state() == QLocalSocket::ConnectedState) {
        if (socket.state() == QLocalSocket::ConnectedState) {
            socket.write(message);
            if (socket.waitForBytesWritten()) {
                if (wait) {
                    socket.waitForReadyRead();
                    std::string response = socket.readAll().toStdString();
                    return response;
                }
                //qCritical() << "Secondary application sent message to IPC server.";
            }
        }
    }
    else {
        qCritical() << "Secondary application cannot connect to IPC server.";
        qCritical() << "Socker error: " << socket.error();
        QCoreApplication::exit();
    }
    return "";
}

#endif // SIOYEK_ANDROID
