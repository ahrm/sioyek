
import QtQuick 2.2
import QtQuick.Controls 2.15

import "qrc:/pdf_viewer/touchui"


Item{

    signal playButtonClicked();
    signal pauseButtonClicked();
    signal stopButtonClicked();
    signal increaseSpeedClicked();
    signal decreaseSpeedClicked();
    signal rateChanged(rate: real);

    id: root
    //color: "yellow"

   // buttons: ["Stop", "Play"]
   property bool isPlaying: true

   Rectangle{
        width: Math.max(Math.min(root.width / 2, root.height / 2), 40);
        height: Math.max(Math.min(root.width / 2, root.height / 2), 40);
        radius: Math.max(Math.min(root.width / 2, root.height / 2), 40);
        color: ma.pressed ? "#222": "black"
        anchors.centerIn: parent
        id: play_button

        Image{
            id: image
            anchors.centerIn: parent
            source: root.isPlaying ? "qrc:/icons/pause.svg" :  "qrc:/icons/play.svg"
            height: 2 * parent.height / 3
            fillMode: Image.PreserveAspectFit

        }

        MouseArea{
            id: ma
            anchors.fill: parent
            onClicked: {
                if (root.isPlaying){
                    root.pauseButtonClicked();
                    isPlaying = false;
                }
                else{
                    root.playButtonClicked();
                    isPlaying = true;
                }
            }
        }
   }

   Rectangle{
        width: Math.max(Math.min(root.width / 4, root.height / 4), 40);
        height: Math.max(Math.min(root.width / 4, root.height / 4), 40);
        radius: Math.max(Math.min(root.width / 4, root.height / 4), 40);
        anchors.right: play_button.left
        anchors.verticalCenter: play_button.verticalCenter
        anchors.margins: 10
        color: ma2.pressed ? "#222": "black"

        Image{
            id: image2
            anchors.centerIn: parent
            source: "qrc:/icons/stop.svg"
            height: 2 * parent.height / 3
            fillMode: Image.PreserveAspectFit

        }

        MouseArea{
            id: ma2
            anchors.fill: parent
            onClicked: {
                root.stopButtonClicked();
            }
        }

   }


   Rectangle{
        width: speed.expanded ? speedslider.width + 20 : Math.max(Math.min(root.width / 4, root.height / 4), 40);
        height: Math.max(Math.min(root.width / 4, root.height / 4), 40);
        radius: Math.max(Math.min(root.width / 4, root.height / 4), 40);
        anchors.left: play_button.right
        anchors.verticalCenter: play_button.verticalCenter
        anchors.margins: 10
        color: ma3.pressed ? "#222": "black"
        id: speed
        property bool expanded: false

        Behavior on width {

            NumberAnimation {
            duration: 500
            easing.type: Easing.OutExpo
            }
        }

        Slider {
            from: -1
            to: 1
            value: _rate
            id: speedslider
            visible: speed.expanded
            width: root.width / 4
            z: 1
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            onPressedChanged: {
                if (pressed == false){
                    speed.expanded = false;
                    root.rateChanged(speedslider.value);
                }
            }
        }

        // Text{
        //     text: "🗣️"
        //     color: "white"
        // 	anchors.centerIn: parent
        //     visible: !speed.expanded
        // }

        Image{
            id: speedimage
            anchors.centerIn: parent
            source: "qrc:/icons/tts.svg"
            height: 2 * parent.height / 3
            fillMode: Image.PreserveAspectFit
            visible: !speed.expanded

        }

        MouseArea{
            id: ma3
            anchors.fill: parent
            onClicked: {
                //root.decreaseSpeedClicked();
                speed.expanded = true;
            }
        }

   }

//    Rectangle{
//         width: Math.max(Math.min(root.width / 4, root.height / 4), 40);
//         height: Math.max(Math.min(root.width / 4, root.height / 4), 40);
//         radius: Math.max(Math.min(root.width / 4, root.height / 4), 40);
//         anchors.left: play_button.right
//         anchors.verticalCenter: play_button.verticalCenter
//         anchors.margins: 10
//         color: ma3.pressed ? "#222": "black"
//         id: decrease

// 		Text{
//             text: "-"
//             color: "white"
// 			anchors.centerIn: parent
// 		}

//         MouseArea{
//             id: ma3
//             anchors.fill: parent
//             onClicked: {
//                 root.decreaseSpeedClicked();
//             }
//         }

//    }

//    Rectangle{
//         width: Math.max(Math.min(root.width / 4, root.height / 4), 40);
//         height: Math.max(Math.min(root.width / 4, root.height / 4), 40);
//         radius: Math.max(Math.min(root.width / 4, root.height / 4), 40);
//         anchors.left: decrease.right
//         anchors.verticalCenter: decrease.verticalCenter
//         anchors.margins: 10
//         color: ma4.pressed ? "#222": "black"
//         id: increase

// 		Text{
//             text: "+"
//             color: "white"
// 			anchors.centerIn: parent
// 		}

//         MouseArea{
//             id: ma4
//             anchors.fill: parent
//             onClicked: {
//                 root.increaseSpeedClicked();
//             }
//         }

//    }
//    Rectangle{
//         anchors.left: increase.right
//         anchors.verticalCenter: increase.verticalCenter
//         anchors.margins: 10
//         width: ratetext.width
//         height: ratetext.height
//         color: "black"

//         Text{
//             id: ratetext
//             text: _rate.toFixed(2)
//             color: "white"
//         }

//    }
    // onButtonClicked: function (index, name){
    //     if (index == 0){
    //         root.stopButtonClicked();
    //     }
    //     if (index == 0){
    //         root.playButtonClicked();
    //     }
    // }
}

