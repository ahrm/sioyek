
import QtQuick 2.2
import QtQuick.Controls 2.15

import "qrc:/pdf_viewer/touchui"


TouchButtonGroup{

    signal genericButtonClicked(index: int, value: string);

    //property list<string> _buttons;

    id: root

    buttons: _buttons

    onButtonClicked: function (index, name){
        root.genericButtonClicked(index, name);
    }
}

