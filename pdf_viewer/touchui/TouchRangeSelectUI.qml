
import QtQuick 2.15
import QtQuick.Controls 2.0

import "qrc:/pdf_viewer/touchui"

Item {
    id: root

    signal confirmPressed(real top, real bottom)
    signal cancelPressed()

    Item{

        anchors.horizontalCenter: parent.horizontalCenter
        z: 10
        y: rangerect.height > 2 * height ? (rangerect.y + rangerect.height / 2 - height / 2) : (rangerect.y > root.height / 2) ? 20 : root.height - height - 20
        width: parent.width / 2
        height: 40
        Behavior on y {

            NumberAnimation {
                duration: 500
                easing.type: Easing.OutExpo
            }
        }
        TouchButtonGroup{
            anchors.fill: parent
            buttons: ["Confirm", "Restore", "Cancel"]
            onButtonClicked: function (index, value){
                         if (index == 0){
                            /* emit */ root.confirmPressed(rangerect.y / root.height * 2 - 1, (rangerect.y + rangerect.height) / root.height * 2 - 1);
                         }
                         if (index == 2){

                            /* emit */ root.cancelPressed();
                                 }
                          if (index == 1){
                                     rangerect.y = root.height / 4;
                                     rangerect.height = root.height / 2;
                                 }
                     }

            //            anchors.centerIn: parent


        }
    }

    Rectangle{
        id: rangerect
        x: 0
        y: (_top + 1) / 2 * root.height
        width: root.width
        height: (_bottom - _top) / 2 * root.height
//        color: "#8800ff00"
        gradient: Gradient {
        GradientStop { position: 0.0; color: "#88FF8400" }
        GradientStop { position: 1.0; color: "#8800B2FF" }
    }

//        function getButtonLocation(){
//            let minIndex = -1;
//            let minDistance = 100000;

//            let candicates = [0, root.height, root.height / 2];
////            for (let candidate of candicates){
//            for (let i = 0; i < candidates.length; i++){
//                let candidate = candidates[i];
//                let distance = Math.min(Math.abs(rangerect.y - candidate), Math.abs((rangerect.y + rangerect.height) - candidate));
//                if (distance < minDistance){
//                    minDistance = distance;
//                    minIndex = i;
//                }
//            }
//            return candidates[minIndex];
//        }


//        Rectangle{
//            id: toprect
//            anchors.left: parent.left
//            anchors.right: parent.right
//            anchors.verticalCenter: parent.top
//            height: 10
//            color: "#FF8400"
//            visible: topmouse.pressed

//        }

//        Rectangle{
//            id: bottomrect
//            anchors.left: parent.left
//            anchors.right: parent.right
//            anchors.verticalCenter: parent.bottom
//            height: 10
//            color: "#00B2FF"
//            visible: bottommouse.pressed

//        }

        Rectangle{
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.top
            width: topmouse.pressed ? root.width + 10 :  10;
            height: 10
            radius: 10
            color: "#FF8400"

            Behavior on width {

                NumberAnimation {
                duration: 200
                easing.type: Easing.OutExpo
                }
            }

            MouseArea{
                anchors.centerIn: parent
                width: parent.width *  6
                height: parent.height * 3
                z: 10
                id: topmouse

                onMouseYChanged: {
                    let actual  = mouseY - parent.height;
                    if (pressed){
                        if ((rangerect.y + actual) < (rangerect.y + rangerect.height)){

                            rangerect.y += actual;
                            rangerect.height -= actual;
                        }

                    }
                }
            }
        }

        Rectangle{
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.bottom
//            width: 10
            width: bottommouse.pressed ? root.width + 10 :  10;
            height: 10
            radius: 10
            z: 10
            color: "#00B2FF"

            Behavior on width {

                NumberAnimation {
                duration: 200
                easing.type: Easing.OutExpo
                }
            }
            MouseArea{
                anchors.centerIn: parent
                width: parent.width * 6
                height: parent.height * 3
                z: 10
                id: bottommouse

                onMouseYChanged: {
                    let actual  = mouseY - parent.height;
                    if (pressed){
                        if (rangerect.height + actual > 0){

//                            rangerect.y += actual;
                            rangerect.height += actual;
                        }

                    }
                }
            }
        }
    }

//    Button{
//        text: "Debug"
//        onClicked: {
//            console.log(rangerect.y);
//        }
//    }


}
