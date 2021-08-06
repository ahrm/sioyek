#ifndef SINGLE_INSTANCE_GUARD_H
#define SINGLE_INSTANCE_GUARD_H

#include <QSharedMemory>

#include <QtNetwork/QLocalServer>
#include <QtNetwork/QLocalSocket>

/**
 * This is an control to guarantee that only one application instance exists at
 * any time.
 * It uses shared memory to check that no more than one instance is running at
 * the same time and also it uses Inter Process Communication (IPC) for a
 * secondary application instance to send parameters to the primary application
 * instance before quitting.
 * An Application must be contructed before the control for signals-slot
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
    explicit RunGuard(const QString &key);
    ~RunGuard();

    bool isPrimary();
    bool isSecondary();
    void sendMessage(const QByteArray &message);

signals:
    void messageReceived(const QByteArray &message);

private slots:
    void onNewConnection();

private:

    const QString key;
    const QString sharedMemLockKey;
    const QString memoryKey;

    QSharedMemory *memory;
    QLocalServer *server = nullptr;

    void readMessage(QLocalSocket *socket);
};

#endif // SINGLE_INSTANCE_GUARD_H
