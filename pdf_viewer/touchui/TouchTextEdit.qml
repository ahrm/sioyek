
import QtQuick 2.2
import QtQuick.Controls 2.15

import "qrc:/pdf_viewer/touchui"

Rectangle {

    color: "black"

    signal confirmed(text: string);
    signal cancelled();
    id: root

    TextInput{
        id: edit
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: 3 * parent.height / 4
        color: "white"
        text: _initialValue
        focus: true
        wrapMode: TextInput.WrapAnywhere
        onAccepted:{
            root.confirmed(edit.text);
        }

    }

    TouchButtonGroup{
        anchors.top: edit.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        buttons: ["Cancel", "Confirm"]
        onButtonClicked: function(index, name){
            if (index == 1){
                root.confirmed(edit.text);
            }
            if (index == 0){
                root.cancelled();
            }
        }
    }

}

