
import QtQuick 2.15
import QtQuick.Controls 2.0

import "qrc:/pdf_viewer/touchui"

Item{
//    color: "white"
    id: root

    signal rectangleSelected(enabled: bool, left: int, right: int, bottom: int, top: int)
//    Text{
//        text: "i am the rect"
//        anchors.centerIn: parent
//    }
//    width: 500
//    height: 500
//    color: "red"

    Rectangle{
        //        z: parent.z + 1
        width: _width
        height: _height
        x: _x
        y: _y
        color: enabler.status ? "#6600ff00" : "#22ff0000"
        property point lastMousePressPosition
//        property int lastMousePressPositionY: 0
//        opacity: 1
        function getLeft(){
            return x;
        }

        function getRight(){
            return x + width;
        }

        function getTop(){
            return y;
        }

        function getBottom(){
            return y + height;
        }

        Behavior on color {

            ColorAnimation {
                //This specifies how long the animation takes
                duration: 200
                //This selects an easing curve to interpolate with, the default is Easing.Linear
//                easing.type: Easing.OutExpo
//                easing.overshoot: 0
//                easing.amplitude: -10
            }
        }

        id: rect

//        Item{
//            width: parent.width / 2
//            height: parent.height / 2
//            MouseArea{
//                id: pan
//                anchors.centerIn: parent
//                onMouseXChanged: {
//                    if (pan.pressed){
//                        rect.x += mouseX
//                        rect.y += mouseY
//                    }
//                }
//            }
//        }

            MouseArea{

                width: parent.width
                height: parent.height
                z: 10
                id: actualpan
                propagateComposedEvents: true
                property real originalX : 0
                property real originalY : 0
//                anchors.fill: parent
//                anchors.centerIn: parent
//                anchors.centerIn: parent
//                width: parent.width
//                height: parent.height

                function boundError(x, y){
                    let maxX = x + rect.width;
                    let minX = x;

                    let maxY = y + rect.height;
                    let minY = y;
//                    console.log("" + minX + " " + maxX + " " + minY + " " + maxY);
                    let errors = [];
                    if (minX < 0){
                        errors.push(-minX);
                    }
                    if (maxX > root.width){
                        errors.push(maxX - root.width);
                    }
                    if (minY < 0){
                        errors.push(-minY);
                    }
                    if (maxY > root.height){
                        errors.push(maxY - root.height);
                    }

                    let error = 0;

                    for (let e of errors){
                        if (e > error){
                            error = e;
                        }
                    }

                    return error;

                }

                onMouseXChanged: {
                    if (actualpan.pressed){
                        let actual = mouseX - parent.width + originalX;
                        let prevError = boundError(rect.x, rect.y);
                        let newError = boundError(rect.x + actual, rect.y);
//                        console.log("" + prevError + " " + newError);
                        if (newError == 0){
                            rect.x += actual;
                        }
                        else{
                            if (actual > 0){

                                rect.x += (actual - newError);
                            }
                            else{

                                rect.x += (actual + newError);
                            }

                        }

                    }

                }

                onMouseYChanged: {
                    if (actualpan.pressed){
                        let actual = mouseY - parent.height + originalY;
                        let prevError = boundError(rect.x, rect.y);
                        let newError = boundError(rect.x, rect.y + actual);
                        if (newError == 0){

                            rect.y += actual;
                        }
                        else{
                            if (actual > 0){
                                rect.y += (actual - newError);
                            }
                            else{
                                rect.y += (actual + newError);
                            }


                        }

                    }
                }
                onPressed: {
                    originalX = parent.width-mouseX ;
                    originalY = parent.height - mouseY;
                    rect.lastMousePressPosition = mapToItem(root, mouse.x, mouse.y);
//                    console.log(rect.lastMousePressPosition);
//                    actualpan.lastMousePressPositionX = mouse.x;
//                    actualpan.lastMousePressPositionY = mouse.y;
//                    console.log(actualpan.lastMousePressPositionX);
//                    console.log(actualpan.lastMousePressPositionY);
//                    mouse.accepted = false;
                }

//                onClicked: {
////                    console.log("clickeD!");
////                    enabler.status = !enabler.status
////                    mouse.accepted = false;
////                    if (pan.contains(mouse)){
////                        console.log("contians");
////                    }
//                }
            }

        Rectangle{
            width: Math.max(Math.min(parent.width / 3, 100), Math.min(parent.width * 2 / 3, 30));
            height: Math.max(Math.min(parent.height / 3, 100), Math.min(parent.height * 2 / 3, 30));
            radius: 50
            color: (pan.pressed ? "white" : "black")
            anchors.centerIn: parent
            property bool status : _enabled
            id: enabler
            Text{
                anchors.centerIn: parent
                color: enabler.status ? "green" : "red"
                text: parent.width > 70 ? (enabler.status ? "enabled" : "disabled") : (enabler.status ? "✔️" : "❌")
            }

            MouseArea{

                id: pan
                width: parent.width
                height: parent.height
//                anchors.fill: parent
//                anchors.centerIn: parent
//                anchors.centerIn: parent
//                width: parent.width
//                height: parent.height

                onMouseXChanged: {
                    if (pan.pressed){
                        let actual = mouseX - parent.width / 2;
                    }

                }

                onMouseYChanged: {
                    if (pan.pressed){
                        let actual = mouseY - parent.height / 2;
                    }
                }
                onClicked: {
//                    console.log("clickeD!");
                    let newPoint = mapToItem(root, mouse.x, mouse.y);
                    let distance = Math.abs(rect.lastMousePressPosition.x - newPoint.x) + Math.abs(rect.lastMousePressPosition.y - newPoint.y);
//                    console.log(Object.keys(newPoint));
//                    console.log(rect.lastMousePressPosition);
//                    console.log(distance);
                    if (distance < 20){

                        enabler.status = !enabler.status
                    }

//                    if (rect.lastMouse)
                }
            }
        }

        Rectangle {
            color: mleft.pressed ? "white" : "black"
            width: 10
            height: 10
            radius: 10
            z: 11
            anchors.horizontalCenter: parent.right
            anchors.verticalCenter: parent.verticalCenter
            visible: rect.getRight() < root.width

            MouseArea{
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
                width: 3 * parent.width
                height: 3 * parent.height
                id: mleft
                onMouseXChanged: {
                    let res =  rect.width + mouseX;
                    if (res > 1){
                        rect.width = rect.width + mouseX
                    }
                }
            }

        }

        Rectangle {
            color: mright.pressed ? "white" : "black"
            width: 10
            height: 10
            radius: 10
            z: 11
            anchors.horizontalCenter: parent.left
            anchors.verticalCenter: parent.verticalCenter
            visible: rect.getLeft() > 0

            MouseArea{
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
                width: 3 * parent.width
                height: 3 * parent.height
                id: mright
                onMouseXChanged: {
                    let res =  rect.width - mouseX;
                    if (res > 1){
                        rect.width = res;
                        rect.x = rect.x + mouseX;

                    }
                }
            }

        }

        Rectangle {
            color: mtop.pressed ? "white" : "black"
            width: 10
            height: 10
            radius: 10
            z: 11
            anchors.verticalCenter: parent.top
            anchors.horizontalCenter: parent.horizontalCenter
            visible: rect.getTop() > 0

            MouseArea{
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
                width: 3 * parent.width
                height: 3 * parent.height
                id: mtop
                onMouseYChanged: {
                    let res =  rect.height - mouseY;
                    if (res > 1){
                        rect.height = res;
                        rect.y = rect.y + mouseY;

                    }
                }
            }

        }

        Rectangle {
            color: mbot.pressed ? "white" : "black"
            width: 10
            height: 10
            radius: 10
            z: 11
            anchors.verticalCenter: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            visible: rect.getBottom() < root.height

            MouseArea{
//                anchors.fill: parent
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
                width: 3 * parent.width
                height: 3 * parent.height
                id: mbot
                onMouseYChanged: {
                    let res =  rect.height + mouseY;
                    if (res > 1){
                        rect.height = res;
//                        rect.y = rect.y + mouseY;

                    }
                }
            }

        }
    }

    Item {
        width: 200
        height: 50
        anchors.horizontalCenter: parent.horizontalCenter
//        Button{
//            anchors.fill: parent
//            text: "test"
//        }
//        MyButtons{
////            buttons: ["one", "two"]
//        }
        TouchButtonGroup{
            anchors.fill: parent
            buttons: ["Confirm", "Restore"]
            onButtonClicked: function(index, name){
                if (index == 0){
                    /* emit */ root.rectangleSelected(enabler.status, rect.getLeft(), rect.getRight(), rect.getTop(), rect.getBottom());
                }

                if (index == 1){
                    rect.x = root.width / 2 - 50
                    rect.y = root.height / 2 - 50
                    rect.width = 100
                    rect.height = 100
                }
            }
        }

            y: (rect.y + rect.height / 2) >= root.height / 2 ? 20 : root.height - height - 20

            Behavior on y {

                NumberAnimation {
                duration: 500
                easing.type: Easing.OutExpo
                }
            }


    }


//    Row{

//        anchors.horizontalCenter: parent.horizontalCenter
//        Button{
//            text: "Confirm"
//            height: 50
//            width: 100

//            y: (rect.y + rect.height / 2) >= root.height / 2 ? 0 : root.height - height

//            Behavior on y {
//                NumberAnimation {
//                //This specifies how long the animation takes
//                duration: 500
//                //This selects an easing curve to interpolate with, the default is Easing.Linear
//                easing.type: Easing.OutExpo
//        //                easing.overshoot: 0
//        //                easing.amplitude: -10
//                }
//            }
//        }



//        Button{
//            text: "Restore"
//            height: 50
//            width: 100

////			anchors.horizontalCenter: parent.horizontalCenter
//            y: (rect.y + rect.height / 2) >= root.height / 2 ? 0 : root.height - height

//            Behavior on y {

//                NumberAnimation {
//                //This specifies how long the animation takes
//                duration: 500
//                //This selects an easing curve to interpolate with, the default is Easing.Linear
//                easing.type: Easing.OutExpo
//        //                easing.overshoot: 0
//        //                easing.amplitude: -10
//                }
//            }
//            onClicked: {
//                rect.x = root.width / 2 - 50
//                rect.y = root.height / 2 - 50
//                rect.width = 100
//                rect.height = 100
//            }
//        }
//    }


//    RRect{

//    }

}
