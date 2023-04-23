
import QtQuick 2.2
import QtQuick.Controls 2.15

import "qrc:/pdf_viewer/touchui"


TouchButtonGroup{

    signal prevButtonClicked();
    signal nextButtonClicked();
    signal initialButtonClicked();
    id: root

    //buttons: ["<-", "initial", "->"]
    buttons: ["qrc:/icons/left.svg","qrc:/icons/home.svg","qrc:/icons/right.svg"]

    onButtonClicked: function (index, name){
        if (index == 0){
            root.prevButtonClicked();
        }
        if (index == 1){
            root.initialButtonClicked();
        }
        if (index == 2){
            root.nextButtonClicked();
        }
    }
}

