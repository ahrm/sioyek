
import QtQuick 2.2
import QtQuick.Controls 2.15

import "qrc:/pdf_viewer/touchui"


Rectangle{
    signal deleteButtonClicked();
    signal editButtonClicked();
    signal changeColorClicked(index: int);

    id: root
    color: "#00ffffff"
    radius: 5
    property bool are_color_buttons_visible: false

    function on_restart(){
        root.are_color_buttons_visible = false;
    } 

    TouchButtonGroup{
        // anchors.right: parent.horizontalCenter
        // anchors.rightMargin: 10
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: Math.min(3 * parent.width / 5, 225)


        id: deletebutton

        //buttons: ["🗑️"]
        buttons: ["Delete", "Edit", ""]
        // color: "black"
        colors: ["black", "black", _current_color]

        visible: !root.are_color_buttons_visible
        onButtonClicked: function (index, name){
            if (index == 0){
                root.deleteButtonClicked();
            }
            if (index == 1){
                root.editButtonClicked();
            }
            if (index == 2){
                root.are_color_buttons_visible = true;
            }
        }
    }

    // Rectangle{
    //     anchors.left: parent.horizontalCenter
    //     anchors.leftMargin: 10
    //     anchors.top: parent.top
    //     anchors.bottom: parent.bottom
    //     width:  Math.min(parent.width / 5, 75)
    //     color: _current_color
    //     radius: 5
    //     id: color_button

    //     Text{
    //         text: "Color"
    //         color: _current_color.hslLightness > 0.5 ? "black" : "white"
    //         anchors.centerIn: parent
    //     }

    //     MouseArea {
    //         anchors.fill: parent
    //         onClicked: {
    //             root.are_color_buttons_visible = !root.are_color_buttons_visible;
    //         }
    //     }


    // }
    Repeater{
        model: 26

        Rectangle{
            required property int index
            anchors.top: get_top_anchor()
            anchors.bottom: get_bottom_anchor()

            width: is_too_small() ? parent.width / 13 : parent.width / 26
            x: is_too_small() ?  (root.are_color_buttons_visible ? (index % 13) * width : root.width / 2 - width / 2) : (root.are_color_buttons_visible ? index * width : root.width / 2 - width / 2)
            color: _colors[index]
            visible: root.are_color_buttons_visible

            function get_top_anchor(){
                if (is_too_small()){
                    if (index < 13){
                        return parent.top;
                    }
                    else{
                        return parent.verticalCenter;
                    }
                }
                else{
                    return parent.top;
                }
            }
            function get_bottom_anchor(){
                if (is_too_small()){
                    if (index < 13){
                        return parent.verticalCenter;
                    }
                    else{
                        return parent.bottom;
                    }
                }
                else{
                    return parent.bottom;
                }

            }
            function is_too_small(){
                return root.width / 26 < 20;
            }

            Behavior on x {

                NumberAnimation {
                    duration: 500
                    easing.type: Easing.OutExpo
                }
            }

            Text{
                anchors.centerIn: parent
                text: String.fromCharCode(97 + index)
                color: _colors[index].hslLightness > 0.5 ? "black" : "white"
            }
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    // root.are_color_buttons_visible = !root.are_color_buttons_visible;
                    root.are_color_buttons_visible = false;
                    root.changeColorClicked(index);
                }
                onPressAndHold: {
                    // root.are_color_buttons_visible = !root.are_color_buttons_visible;
                    root.are_color_buttons_visible = false;
                    root.changeColorClicked(index + 26);
                }
            }
        }

    }
    // TouchButtonGroup{
    //     anchors.right: parent.right
    //     anchors.top: parent.top
    //     anchors.bottom: parent.bottom
    //     width: 4 * parent.width / 5

    //     buttons: Array(_colors.length).fill("")
    //     colors: _colors

    //     onButtonClicked: function (index, name){
    //         root.changeColorClicked(index);
    //     }
    // }

    // Row{
    // 	anchors.top: parent.top
    // 	anchors.bottom: parent.bottom
    // 	//anchors.left: deletebutton.right
    // 	anchors.right: parent.right
    // 	anchors.margins: 2
    // 	width: parent.width * 3 / 4
    // 	layoutDirection: Qt.RightToLeft
    // 	spacing: parent.width / 50

    // 	Repeater{

    // 		model: 4

    // 		Rectangle{
    // 			//anchors.right: color_b.left
    // 			//anchors.verticalCenter: parent.verticalCenter
    // 			width: Math.min(parent.width / 4, parent.height / 1.1)
    // 			height: width
    // 			radius: Math.min(width,height)
    // 			anchors.verticalCenter: parent.verticalCenter
    // 			//id: color_a
    // 			color: _colors[3-index]
    // 			opacity: (3-index) == _index ? 1 : 0.5
    // 			Text{
    // 				text: String.fromCharCode(97 + (3 - index))
    // 				color: "white"
    // 				//anchors.centerIn: parent
    // 				//anchors.fill: parent
    // 				anchors.centerIn: parent
    // 				z: 10
    // 			}
    // 			MouseArea{
    // 				anchors.fill: parent
    // 				onClicked: {
    // 					root.changeColorClicked(3- index);
    // 				}
    // 			}
    // 		}
    // 	}
    // }
}

