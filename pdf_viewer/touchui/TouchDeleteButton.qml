
import QtQuick 2.2
import QtQuick.Controls 2.15

import "qrc:/pdf_viewer/touchui"


TouchButtonGroup{

    signal deleteButtonClicked();

    id: root

    buttons: ["delete"]
    color: "red"

    onButtonClicked: function (index, name){
        if (index == 0){
            root.deleteButtonClicked();
        }
    }
}

