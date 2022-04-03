#ifndef OPEN_WITH_APP_H
#define OPEN_WITH_APP_H

#include <qapplication.h>

class OpenWithApplication : public QApplication
{
	Q_OBJECT
public:
	QString file_name;
    OpenWithApplication(int &argc, char **argv)
        : QApplication(argc, argv)
    {
    }
signals:
    void fileReady(QString fn);

protected:
    bool event(QEvent *event) override;
};

#endif // OPEN_WITH_APP_H