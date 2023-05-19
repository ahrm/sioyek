#include "touchui/TouchMainMenu.h"
#include "qqmlengine.h"


TouchMainMenu::TouchMainMenu(bool fit_mode, bool portaling, bool fullscreen, bool ruler, bool speaking, bool locked, int current_colorscheme_index, QWidget* parent) : QWidget(parent){

    setAttribute(Qt::WA_NoMousePropagation);

    quick_widget = new QQuickWidget(this);

    quick_widget->setResizeMode(QQuickWidget::ResizeMode::SizeRootObjectToView);
    quick_widget->setAttribute(Qt::WA_AlwaysStackOnTop);
    quick_widget->setClearColor(Qt::transparent);

    quick_widget->rootContext()->setContextProperty("_currentColorschemeIndex", current_colorscheme_index);
    quick_widget->rootContext()->setContextProperty("_locked", locked);
    quick_widget->rootContext()->setContextProperty("_fullscreen", fullscreen);
    quick_widget->rootContext()->setContextProperty("_ruler", ruler);
    quick_widget->rootContext()->setContextProperty("_speaking", speaking);
    quick_widget->rootContext()->setContextProperty("_portaling", portaling);
    quick_widget->rootContext()->setContextProperty("_fit", fit_mode);

    quick_widget->setSource(QUrl("qrc:/pdf_viewer/touchui/TouchMainMenu.qml"));


    QObject::connect(
                dynamic_cast<QObject*>(quick_widget->rootObject()),
                SIGNAL(selectTextClicked()),
                this,
                SLOT(handleSelectText()));

    QObject::connect(
                dynamic_cast<QObject*>(quick_widget->rootObject()),
                SIGNAL(gotoPageClicked()),
                this,
                SLOT(handleGotoPage()));

    QObject::connect(
                dynamic_cast<QObject*>(quick_widget->rootObject()),
                SIGNAL(tocClicked()),
                this,
                SLOT(handleToc()));

    QObject::connect(
                dynamic_cast<QObject*>(quick_widget->rootObject()),
                SIGNAL(searchClicked()),
                this,
                SLOT(handleSearch()));

    QObject::connect(
                dynamic_cast<QObject*>(quick_widget->rootObject()),
                SIGNAL(fullscreenClicked()),
                this,
                SLOT(handleFullscreen()));

    QObject::connect(
                dynamic_cast<QObject*>(quick_widget->rootObject()),
                SIGNAL(bookmarksClicked()),
                this,
                SLOT(handleBookmarks()));

    QObject::connect(
                dynamic_cast<QObject*>(quick_widget->rootObject()),
                SIGNAL(highlightsClicked()),
                this,
                SLOT(handleHighlights()));

    QObject::connect(
                dynamic_cast<QObject*>(quick_widget->rootObject()),
                SIGNAL(rulerModeClicked()),
                this,
                SLOT(handleRuler()));

    QObject::connect(
                dynamic_cast<QObject*>(quick_widget->rootObject()),
                SIGNAL(lightColorschemeClicked()),
                this,
                SLOT(handleLightColorscheme()));

    QObject::connect(
                dynamic_cast<QObject*>(quick_widget->rootObject()),
                SIGNAL(darkColorschemeClicked()),
                this,
                SLOT(handleDarkColorscheme()));

    QObject::connect(
                dynamic_cast<QObject*>(quick_widget->rootObject()),
                SIGNAL(customColorschemeClicked()),
                this,
                SLOT(handleCustomColorscheme()));

    QObject::connect(
                dynamic_cast<QObject*>(quick_widget->rootObject()),
                SIGNAL(openPrevDocClicked()),
                this,
                SLOT(handleOpenPrevDoc()));

    QObject::connect(
                dynamic_cast<QObject*>(quick_widget->rootObject()),
                SIGNAL(openNewDocClicked()),
                this,
                SLOT(handleOpenNewDoc()));


    QObject::connect(
                dynamic_cast<QObject*>(quick_widget->rootObject()),
                SIGNAL(commandsClicked()),
                this,
                SLOT(handleCommands()));

    QObject::connect(
                dynamic_cast<QObject*>(quick_widget->rootObject()),
                SIGNAL(settingsClicked()),
                this,
                SLOT(handleSettings()));

    QObject::connect(
                dynamic_cast<QObject*>(quick_widget->rootObject()),
                SIGNAL(addBookmarkClicked()),
                this,
                SLOT(handleAddBookmark()));

    QObject::connect(
                dynamic_cast<QObject*>(quick_widget->rootObject()),
                SIGNAL(portalClicked()),
                this,
                SLOT(handlePortal()));

    QObject::connect(
                dynamic_cast<QObject*>(quick_widget->rootObject()),
                SIGNAL(deletePortalClicked()),
                this,
                SLOT(handleDeletePortal()));

    QObject::connect(
                dynamic_cast<QObject*>(quick_widget->rootObject()),
                SIGNAL(globalBookmarksClicked()),
                this,
                SLOT(handleGlobalBookmarks()));

    QObject::connect(
                dynamic_cast<QObject*>(quick_widget->rootObject()),
                SIGNAL(globalHighlightsClicked()),
                this,
                SLOT(handleGlobalHighlights()));

    QObject::connect(
                dynamic_cast<QObject*>(quick_widget->rootObject()),
                SIGNAL(ttsClicked()),
                this,
                SLOT(handleTTS()));

    QObject::connect(
                dynamic_cast<QObject*>(quick_widget->rootObject()),
                SIGNAL(horizontalLockClicked()),
                this,
                SLOT(handleHorizontalLock()));

    QObject::connect(
                dynamic_cast<QObject*>(quick_widget->rootObject()),
                SIGNAL(fitToPageWidthClicked()),
                this,
                SLOT(handleFitToPageWidth()));

    QObject::connect(
                dynamic_cast<QObject*>(quick_widget->rootObject()),
                SIGNAL(drawingModeButtonClicked()),
                this,
                SLOT(handleDrawingMode()));

    QObject::connect(
                dynamic_cast<QObject*>(quick_widget->rootObject()),
                SIGNAL(downloadPaperClicked()),
                this,
                SLOT(handleDownloadPaper()));
}

void TouchMainMenu::handleDrawingMode(){
    emit drawingModeClicked();
}

void TouchMainMenu::handleSelectText(){
    emit selectTextClicked();
}

void TouchMainMenu::handleGotoPage(){
    emit gotoPageClicked();
}

void TouchMainMenu::handleToc(){
    emit tocClicked();
}

void TouchMainMenu::handleDownloadPaper(){
    emit downloadPaperClicked();
}


void TouchMainMenu::handleSearch(){
    emit searchClicked();
}

void TouchMainMenu::handleFullscreen(){
    emit fullscreenClicked();
}

void TouchMainMenu::handleBookmarks(){
    emit bookmarksClicked();
}

void TouchMainMenu::handleHighlights(){
    emit highlightsClicked();
}

void TouchMainMenu::handleRuler(){
    emit rulerModeClicked();
}

void TouchMainMenu::handleLightColorscheme(){
    emit lightColorschemeClicked();
}

void TouchMainMenu::handleDarkColorscheme(){
    emit darkColorschemeClicked();
}

void TouchMainMenu::handleCustomColorscheme(){
    emit customColorschemeClicked();
}

void TouchMainMenu::handleOpenPrevDoc(){
    emit openPrevDocClicked();
}

void TouchMainMenu::handleOpenNewDoc(){
    emit openNewDocClicked();
}

void TouchMainMenu::handleCommands(){
    emit commandsClicked();
}

void TouchMainMenu::handleSettings(){
    emit settingsClicked();
}

void TouchMainMenu::handleAddBookmark(){
    emit addBookmarkClicked();
}

void TouchMainMenu::handlePortal(){
    emit portalClicked();
}

void TouchMainMenu::handleDeletePortal(){
    emit deletePortalClicked();
}

void TouchMainMenu::handleGlobalBookmarks(){
    emit globalBookmarksClicked();
}

void TouchMainMenu::handleGlobalHighlights(){
    emit globalHighlightsClicked();
}

void TouchMainMenu::handleTTS(){
    emit ttsClicked();
}

void TouchMainMenu::handleFitToPageWidth(){
    emit fitToPageWidthClicked();
}

void TouchMainMenu::handleHorizontalLock(){
    emit horizontalLockClicked();
}
//void TouchMainMenu::handleSelect(int item) {
//    emit pageSelected(item);
//}

void TouchMainMenu::resizeEvent(QResizeEvent* resize_event){
    quick_widget->resize(resize_event->size().width(), resize_event->size().height());
    QWidget::resizeEvent(resize_event);

}
