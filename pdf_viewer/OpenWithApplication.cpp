#include <OpenWithApplication.h>

bool OpenWithApplication::event(QEvent *event) {
	if (event->type() == QEvent::FileOpen) {
		QFileOpenEvent *openEvent = static_cast<QFileOpenEvent *>(event);
		emit file_ready(openEvent->file());
	}

	return QApplication::event(event);
}
