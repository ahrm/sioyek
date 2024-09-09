

import QtQuick 2.2
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.1

//import com.myself 1.0

RowLayout{

    component GroupButton : Item{
        property string text: ""
        //        property alias onClicked: msa.onClicked
        property bool roundLeft : false
        property bool roundRight : false
        property var gbButtonClicked : null
        property color color: "black"
        property color selectedColor: color.tint("#44ffffff")
        property color disabledColor: "red"
        property bool isSelected: false
        property int mode: -1;

        //        property

        //        property alias Layout.fillWidth: mainrect.

        signal clicked()
        signal pressAndHold()
        signal released()

        id: gb

        Rectangle{
            anchors.fill: parent
//            color: button_area.pressed ? "#444444" : "black"
            function getColor(){
                if (mode >= 0){
                    if (mode == 0){
                        if (button_area.pressed){
                            return  gb.color.tint("#22ffffff");
                        }
                        else{
                            return gb.color;
                        }

                    }
                    else if (mode == 1){
                        if (button_area.pressed){
                            return  gb.color.tint("#5500ff00");
                        }
                        else{
                            return  gb.color.tint("#4400ff00");
                        }
                    }
                    else {
                        if (button_area.pressed){
                            return  gb.color.tint("#55ff0000");
                        }
                        else{
                            return  gb.color.tint("#44ff0000");
                        }
                    }
                }
                else{
                    if (gb.isSelected){
                        if (button_area.pressed){
                            return  gb.selectedColor.tint("#22ffffff");
                        }
                        else{
                            return gb.selectedColor;
                        }

                    }
                    else{
                        if (button_area.pressed){
                            return  gb.color.tint("#22ffffff");
                        }
                        else{
                            return gb.color;
                        }
                    }
                }
            }

            color: getColor()
            radius: 4
            id: mainrect

            Text{
                id: text
                text: gb.text
                color: "white"
                anchors.centerIn: parent
                visible: !gb.text.startsWith("qrc:/")
            }
            Image{

                id: image
                source: gb.text.startsWith("qrc:/") ? gb.text : ""
                visible: gb.text.startsWith("qrc:/")
                anchors.centerIn: parent
                height: 2 * parent.height / 3
                fillMode: Image.PreserveAspectFit

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
                id: button_area
                onPressAndHold: {
                    /* emit */ gb.pressAndHold();
                }
                onReleased: {
                    /* emit */ gb.released();
                }

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
    property list<string> tips
    property bool radio: false
    property bool modal: false
    property int selectedIndex : -1
    property list<int> modeList : []
    property color color: "black"
    property bool roundLeft: true
    property bool roundRight: true
    property list<color> colors: []
    //        property var onButtonClicked : null

    spacing: 0
//    anchors.fill: parent
    id: row

    signal buttonClicked(int index, string value)

    Repeater{
        model: buttons

        Item{
            //                anchors.fill: parent
            Layout.fillWidth: true
            Layout.preferredWidth: 100
            Layout.fillHeight: true
            //                height: parent.height
            //anchors.top: parent.top
            //anchors.bottom: parent.bottom
            property string originalLabel: ""
            property int originalPreferredWidth: -1

            id: x

            GroupButton{
                anchors.fill: parent
                text: model.modelData
                roundLeft: (index == 0) && row.roundLeft
                roundRight: index == (row.buttons.length - 1) && row.roundRight
                isSelected: index == selectedIndex
                mode: modeList.length > 0 ? modeList[index] : -1
                color: row.colors.length == 0 ? row.color : row.colors[index]
//                color: "red"
                onPressAndHold: {
                    if (row.tips.length == row.buttons.length){
                        x.originalLabel = text;
                        x.originalPreferredWidth = x.Layout.preferredWidth;

                        let tip = row.tips[index];
                        text = tip;
                        x.Layout.preferredWidth = 1400;
                    }
                }

                gbButtonClicked: function() {
                    //wconsole.log(row.modeList[index]);

                    if (row.radio){
                        row.selectedIndex = index;
                    }

                    if (row.modal){
                        row.modeList[index] = (row.modeList[index] + 1) % 3;
                    }

                    /* emit */ row.buttonClicked(index, row.buttons[index]);
                }
                onReleased: {
                    if (x.originalPreferredWidth != -1){
                        x.Layout.preferredWidth = x.originalPreferredWidth;
                        text = x.originalLabel;
                    }
                    x.originalPreferredWidth = -1;
                    x.originalLabel = "";
                }

            }

            Behavior on Layout.preferredWidth {

                NumberAnimation {
                    duration: 200
                    easing.type: Easing.OutExpo
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
