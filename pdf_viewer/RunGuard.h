#ifndef SIOYEK_ANDROID
#ifndef SINGLE_INSTANCE_GUARD_H
#define SINGLE_INSTANCE_GUARD_H

#include <QLockFile>

#include <QtNetwork/QLocalServer>
#include <QtNetwork/QLocalSocket>

#include <functional>
#include <memory>
/**
 * Guarantees that only one application instance with a given key runs at a
 * time, and lets a secondary instance forward its arguments to the primary
 * one over a local socket before quitting.
 *
 * The primary instance is whoever holds a lock file (QLockFile). Both the lock
 * file and the socket live in a per-user directory that the OS does not purge
 * while the application is running:
 *
 *   Linux and other Unix:  $XDG_RUNTIME_DIR/sioyek
 *   macOS:                 ~/Library/Application Support/sioyek
 *   Windows:               the lock file in the temp directory; the socket is
 *                          a named pipe, which has no file.
 *
 * Earlier versions used QSharedMemory instead of a lock file. On Unix that is
 * a System V segment whose key is derived from a file in the temp directory.
 * macOS deletes temp files that have not been accessed for a few days, so
 * after a long-running instance lost that file, the next launch could not see
 * it and became a second primary. Crashes also left orphaned segments behind.
 * QLockFile records the owner's PID, so a lock left by a crashed process is
 * detected as stale and taken over.
 *
 * An Application must be constructed before the guard for signal-slot
 * communication to work.
 *
 * Usage example:
 *
 * int main(int argc, char *argv[])
 * {
 *     QApplication app{argc, argv};
 *
 *     ...
 *
 *     RunGuard guard{"Lentigram"};
 *     if (guard.isPrimary()) {
 *         QObject::connect(
 *             &guard,
 *             &RunGuard::messageReceived, [this](const QByteArray &message) {
 *
 *                 ...process message coming from secondary application...
 *
 *                 qDebug() << message;
 *             }
 *         );
 *     } else {
 *         guard.sendMessage(app.arguments().join(' ').toUtf8());
 *         return 0;
 *     }
 *
 *     ...
 *
 *     app.exec();
 * }
 *
 * This code is inspired by the following:
 * https://stackoverflow.com/questions/5006547/qt-best-practice-for-a-single-instance-app-protection
 * https://github.com/itay-grudev/SingleApplication
 */
class RunGuard : public QObject
{
    Q_OBJECT

public:
    std::function<void(QLocalSocket*)> on_delete;
    explicit RunGuard(const QString& key);
    ~RunGuard();

    bool isPrimary();
    bool isSecondary();
    std::string sendMessage(const QByteArray& message, bool wait=false);

signals:
    void messageReceived(const QByteArray& message, QLocalSocket* socket);

private slots:
    void onNewConnection();

private:

    const QString key;
    QString serverName;

    std::unique_ptr<QLockFile> lockFile;
    bool primary = false;
    QLocalServer* server = nullptr;

    void readMessage(QLocalSocket* socket);
};

#endif // SINGLE_INSTANCE_GUARD_H

#endif // SIOYEK_ANDROID
