
import QtQuick 2.2
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.1

//import com.myself 1.0

RowLayout{

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

