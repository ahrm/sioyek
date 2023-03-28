
import QtQuick 2.2
import QtQuick.Controls 2.15



Rectangle {
    color: "#00000000"
    id: root

    signal valueSelected(val: bool)

    Row{
        id: rowid

        Label{
            text: _name
        }

        CheckBox{
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

