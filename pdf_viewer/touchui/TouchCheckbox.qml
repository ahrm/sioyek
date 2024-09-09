
import QtQuick 2.2
import QtQuick.Controls 2.15



Rectangle {
    color: "#444"
    id: root
    radius: 10

    signal valueSelected(val: bool)

    Item{
        id: rowid
        anchors.centerIn: parent
        width: lbl.width + checkbox.width
        //width: parent.width / 2
        height: parent.height / 2

        Label{
            id: lbl
            text: _name
            color: "white"
            anchors.left: checkbox.right
            anchors.verticalCenter: checkbox.verticalCenter
        }


        CheckBox{
            //anchors.centerIn: parent
            id: checkbox
            checked: _initialValue
        }

        anchors {
            left: parent.left
            right: parent.right
            verticalCenter: parent.verticalCenter
        }
    }

    Button{
        text: "Confirm"
        anchors {
            horizontalCenter: parent.horizontalCenter
            top: rowid.bottom
        }

        onClicked: {
            root.valueSelected(checkbox.checked);
            //                console.log("something has happened!");
        }
    }

}

