
import QtQuick 2.2
import QtQuick.Controls 2.15

import "qrc:/pdf_viewer/touchui"


Rectangle{
    signal deleteButtonClicked();
    signal editButtonClicked();
    signal changeColorClicked(index: int);

    id: root
    color: "#00ffffff"
    radius: 5
    property bool are_color_buttons_visible: false

    function on_restart(){
        root.are_color_buttons_visible = false;
    } 

    TouchButtonGroup{
        // anchors.right: parent.horizontalCenter
        // anchors.rightMargin: 10
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: Math.min(3 * parent.width / 5, 225)


        id: deletebutton

        //buttons: ["🗑️"]
        buttons: ["Delete", "Edit", ""]
        // color: "black"
        colors: ["black", "black", _current_color]

        visible: !root.are_color_buttons_visible
        onButtonClicked: function (index, name){
            if (index == 0){
                root.deleteButtonClicked();
            }
            if (index == 1){
                root.editButtonClicked();
            }
            if (index == 2){
                root.are_color_buttons_visible = true;
            }
        }
    }

    TouchSymbolColorSelector{
        colors: _colors
        anchors.fill: parent
        visible: root.are_color_buttons_visible

        onColorClicked: function(index){
            root.changeColorClicked(index);
            root.are_color_buttons_visible = !root.are_color_buttons_visible;
        }

        onColorHeld: function(index){
            root.changeColorClicked(index + 26);
            root.are_color_buttons_visible = !root.are_color_buttons_visible;
        }
    }
}

