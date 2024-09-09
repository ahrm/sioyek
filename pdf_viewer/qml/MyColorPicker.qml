import QtQuick 2.2
import QtQuick.Controls 2.15
//import QtQuick.Dialogs 1.0
//import QtQuick.Dialogs 1.3
import Qt.labs.platform 1.1

//ColorDialog{

//}

//Slider{

//}
//Rectangle{
//    color: "black"
////    anchors.fill: parent
//    width: 500
//    height: 500

Item{

    ColorDialog {
        id: colorDialog
        title: "Please choose a color"
        onAccepted: {
        console.log("You chose: " + colorDialog.color)
        Qt.quit()
        }
        onRejected: {
        console.log("Canceled")
        Qt.quit()
        }
        Component.onCompleted: visible = true
    }
}

//    Slider {
//        from: 1
//        value: 25
//        to: 100
//    }
//}


//colorDialog {
//    id: colorDialog
//    title: "Please choose a color"
//    onAccepted: {
//        console.log("You chose: " + colorDialog.color)
//        Qt.quit()
//    }
//    onRejected: {
//        console.log("Canceled")
//        Qt.quit()
//    }
//    Component.onCompleted: visible = true
//}

//Rectangle {
//    width: 1000
//    height: 1000
//    color: "yellow"
//}
