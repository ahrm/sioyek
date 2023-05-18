
import QtQuick 2.2
import QtQuick.Controls 2.15

Rectangle {
//    anchors.fill: parent
//    height: 300
//    width: parent.width
//    width: 100
//    height: 100

//    implicitWidth: 100
//    implicitHeight: 100
    //    anchors.fill: parent
    //color: "#00000000"
    color: "#444"
    radius: 10
    //color: "black"
    //    flags:  Qt.WA_TranslucentBackground | Qt.WA_AlwaysStackOnTop
    //    flags:  Qt.WA_AlwaysStackOnTop
    id: root

    signal valueSelected(val: int)
    signal canceled()

    Slider{
        from: _from
        to: _to
        id: my_slider
        value: _initialValue

        anchors {
            left: parent.left
            right: parent.right
            verticalCenter: parent.verticalCenter
            leftMargin: 10
            rightMargin: 10
        }



    }

    Row{

        anchors {
            horizontalCenter: parent.horizontalCenter
            top: my_slider.bottom
        }
        Button{
            text: "Confirm"

            onClicked: {
                root.valueSelected(my_slider.value);
            }
        }
        Button{
            text: "Cancel"

            onClicked: {
                root.canceled();
            }
        }
    }

}


//Slider{
//    width: 300
//    height: 300

//    from: _from
//    to: _to
//    id: my_slider
//    value: _initialValue

//    signal valueSelected(val: int)

////    anchors {
////        left: parent.left
////        right: parent.right
////        verticalCenter: parent.verticalCenter
////    }


//    Button{
//        text: "Confirm"
//        anchors {
//            horizontalCenter: parent.horizontalCenter
//            top: my_slider.bottom
//        }

//        onClicked: {
//            my_slider.valueSelected(my_slider.value);
//            //                console.log("something has happened!");
//        }
//    }

//}
