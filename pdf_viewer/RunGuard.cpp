#ifndef SIOYEK_ANDROID

#include "RunGuard.h"

#include <QCryptographicHash>
#include <QDeadlineTimer>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QThread>
#include <QtCore>

namespace
{
    // Directory holding the lock file and, on Unix, the socket. It must be
    // per-user and must not be purged while sioyek runs: macOS deletes files in
    // $TMPDIR that have not been accessed for a few days, and systemd-tmpfiles
    // ages out /tmp on many Linux distributions.
    QString ipcDirectory()
    {
#ifdef Q_OS_WIN
        // The lock file stays open while it is held, so Windows cannot delete
        // it from the temp directory.
        return QDir::tempPath();
#else
        // $XDG_RUNTIME_DIR on Linux, ~/Library/Application Support on macOS.
        QString base = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
        if (base.isEmpty()) {
            base = QDir::tempPath();
        }
        QString dir = base + "/sioyek";
        QDir().mkpath(dir);
        return dir;
#endif
    }

    // A short, filesystem-safe name derived from the instance key. Instance
    // keys can be long (sioyek_<uuid>), and Unix socket paths are limited.
    QString fileStem(const QString& key)
    {
        QByteArray hash = QCryptographicHash::hash(key.toUtf8(), QCryptographicHash::Sha1).toHex();
        return "ipc-" + QString::fromLatin1(hash.left(16));
    }
}

RunGuard::RunGuard(const QString& key) : QObject{},
key(key)
{
    QString dir = ipcDirectory();
    QString stem = fileStem(key);

    lockFile = std::make_unique<QLockFile>(dir + "/" + stem + ".lock");
    // A lock is stale only when the process that holds it no longer exists,
    // never because of its age: a viewer can stay open for weeks.
    lockFile->setStaleLockTime(0);

#ifdef Q_OS_WIN
    serverName = key;
#else
    serverName = dir + "/" + stem + ".sock";
    // sockaddr_un.sun_path holds 104 bytes on macOS and 108 on Linux. With a
    // very long home directory, fall back to Qt's default socket location.
    if (QFile::encodeName(serverName).size() >= 100) {
        serverName = key;
    }
#endif

    if (lockFile->tryLock(0)) {
        primary = true;
    }
    else if (lockFile->error() != QLockFile::LockFailedError) {
        // We cannot even create the lock file. Starting is more useful than
        // refusing to, even if it means a second instance.
        qWarning() << "Cannot create the single-instance lock file" << lockFile->fileName()
                   << "; starting as the primary instance anyway.";
        primary = true;
    }

    if (primary) {
        // Any socket left at this path belongs to a primary that is gone,
        // since we now hold the lock.
        QLocalServer::removeServer(serverName);
        server = new QLocalServer(this);
        server->setSocketOptions(QLocalServer::UserAccessOption);
        if (!server->listen(serverName)) {
            qCritical() << "Cannot start IPC server:" << server->errorString();
        }
        QObject::connect(server, &QLocalServer::newConnection,
            this, &RunGuard::onNewConnection);
    }
}

RunGuard::~RunGuard()
{
    if (server) {
        // Removes the socket file on Unix.
        server->close();
        delete server;
        server = nullptr;
    }
    if (lockFile) {
        lockFile->unlock();
    }
}

bool RunGuard::isPrimary()
{
    return primary;
}

bool RunGuard::isSecondary()
{
    return !primary;
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

    // The primary takes the lock before it starts listening, so a launch that
    // races a starting primary can briefly find no server. Retry for a while.
    QDeadlineTimer deadline(5000);
    while (true) {
        socket.connectToServer(serverName, QLocalSocket::ReadWrite);
        if (socket.waitForConnected(1000)) {
            break;
        }
        if (deadline.hasExpired()) {
            qCritical() << "Secondary application cannot connect to IPC server.";
            qCritical() << "Socket error: " << socket.error();
            return "";
        }
        socket.abort();
        QThread::msleep(100);
    }

    socket.write(message);
    if (socket.waitForBytesWritten()) {
        if (wait) {
            socket.waitForReadyRead();
            std::string response = socket.readAll().toStdString();
            return response;
        }
    }
    return "";
}

#endif // SIOYEK_ANDROID
