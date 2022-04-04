#ifndef OPEN_WITH_APP_H
#define OPEN_WITH_APP_H

#include <QApplication>
#include <QFileOpenEvent>

class OpenWithApplication : public QApplication
{
	Q_OBJECT
public:
    OpenWithApplication(int &argc, char **argv)
        : QApplication(argc, argv)
    {
    }
signals:
    void file_ready(const QString& file_name);

protected:
    bool event(QEvent *event) override;
};

#endif // OPEN_WITH_APP_H