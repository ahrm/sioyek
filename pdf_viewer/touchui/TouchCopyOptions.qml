
import QtQuick 2.2
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.1

//import com.myself 1.0

Item{

    component GroupButton : Item{
        property alias text: text.text
//        property alias onClicked: msa.onClicked
        property bool roundLeft : false
        property bool roundRight : false
        property var gbButtonClicked : null
//        property

//        property alias Layout.fillWidth: mainrect.

        signal clicked()

        id: gb

        Rectangle{
            anchors.fill: parent
            color: "black"
            radius: 4
            id: mainrect

            Text{
                id: text
                color: "white"
                anchors.centerIn: parent
            }
            Rectangle{
                anchors.left: mainrect.right
                anchors.leftMargin: -mainrect.radius
                color: mainrect.color
                width: mainrect.radius
                height: mainrect.height
                visible: !gb.roundRight
            }

            Rectangle{
                anchors.right: mainrect.left
                anchors.rightMargin: -mainrect.radius
                color: mainrect.color
                width: mainrect.radius
                height: mainrect.height
                visible: !gb.roundLeft
            }

            MouseArea{
                anchors.fill: parent
                onClicked: {
                    if (gb.gbButtonClicked){
                        gb.gbButtonClicked();
                    }
                    else{
                        console.log("this should not happen");
                    }

                    /* emit */ gb.clicked();
                }
            }
        }
    }

    component ButtonGroup : RowLayout{
        property list<string> buttons
//        property var onButtonClicked : null

        spacing: 0
        anchors.fill: parent
        id: row

        signal buttonClicked(int index, string value)

        Repeater{
            model: buttons

            Item{
//                anchors.fill: parent
                Layout.fillWidth: true
//                height: parent.height
                anchors.top: parent.top
                anchors.bottom: parent.bottom

                GroupButton{
                    anchors.fill: parent
                    text: model.modelData
                    roundLeft: index == 0
                    roundRight: index == row.buttons.length - 1
                    gbButtonClicked: function() {

                        /* emit */ row.buttonClicked(index, row.buttons[index]);
                    }

                }

                Rectangle{
                    width: 1
                    height: parent.height
                    color: "#222"
                    visible: (index > 0) && (index < row.buttons.length)
                }
            }


       }

   }

    id: root
//    width: 300
    height: 50

    signal copyPressed()
    signal scholarPressed()
    signal googlePressed()
    signal highlightPressed()

    ButtonGroup{
        anchors.fill: parent

        buttons: ["Copy", "Scholar", "Google", "Highlight"]
        onButtonClicked: function(index, val) {
            if (index == 0){
                root.copyPressed();
            }

            if (index == 1){
                root.scholarPressed();
            }

            if (index == 2){
                root.googlePressed();
            }

            if (index == 3){
                root.highlightPressed();
            }

        }

    }

}
//import QtQuick 2.2
//import QtQuick.Controls 2.15

////import com.myself 1.0



//Item{
////        anchors.fill: parent
////        width: inner.width
////    width: 500
//    height: inner.height

//    id: root

//    signal copyPressed();
//    signal scholarPressed();
//    signal googlePressed();
//    signal highlightPressed();

//    Row{
//        id: inner
//        anchors.centerIn: parent

//        Rectangle{
//            width: 70
//            height: 40
//            color: "black"
//            radius: 4
//            Text{
////                anchors.verticalCenter: parent.verticalCenter
//                anchors.centerIn: parent
//                text: "Copy"
//                color: "white"
//            }
//            Rectangle{
//                anchors.left: parent.right
//                anchors.leftMargin: -parent.radius
//                width: parent.radius
//                height: parent.height
//                color: "black"
//            }
//            MouseArea{
//                width: parent.width
//                height: parent.height
//                onClicked: {
//                    root.copyPressed();
//                }
//            }
//        }

//        Rectangle{
//            width: 1
//            height: 40
//            color: "#222"
//        }

//        Rectangle{
//            width: 70
//            height: 40
//            color: "black"
//            Text{
////                anchors.verticalCenter: parent.verticalCenter
//                anchors.centerIn: parent
//                text: "Scholar"
//                color: "white"
//            }
//            MouseArea{
//                width: parent.width
//                height: parent.height
//                onClicked: {
//                    root.scholarPressed();
//                }
//            }
//        }

//        Rectangle{
//            width: 1
//            height: 40
//            color: "#222"
//        }

//        Rectangle{
//            width: 70
//            height: 40
//            color: "black"
//            Text{
////                anchors.verticalCenter: parent.verticalCenter
//                anchors.centerIn: parent
//                text: "Google"
//                color: "white"
//            }
//            MouseArea{
//                width: parent.width
//                height: parent.height
//                onClicked: {
//                    root.googlePressed();
//                }
//            }
//        }

//        Rectangle{
//            width: 1
//            height: 40
//            color: "#222"
//        }

//        Rectangle{
//            width: 70
//            height: 40
//            color: "black"
//            radius: 4
//            Text{
////                anchors.verticalCenter: parent.verticalCenter
//                anchors.centerIn: parent
//                text: "Highlight"
//                color: "white"
//            }
//            Rectangle{
//                anchors.right: parent.left
//                anchors.rightMargin: -parent.radius
//                width: parent.radius
//                height: parent.height
//                color: "black"
//            }

//            MouseArea{
//                width: parent.width
//                height: parent.height
//                onClicked: {
//                    root.highlightPressed();
//                }
//            }
//        }

//    }

//}
