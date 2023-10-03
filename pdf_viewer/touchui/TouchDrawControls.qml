
import QtQuick 2.2
import QtQuick.Controls 2.15

import "qrc:/pdf_viewer/touchui"


Rectangle{
    signal exitDrawModeButtonClicked();
    signal changeColorClicked(index: int);
    signal enablePenDrawModeClicked();
    signal disablePenDrawModeClicked();
    signal eraserClicked();
    signal penSizeChanged(size: real);

    id: root
    //color: "black"
    color: "#00ffffff"
    radius: 5
    property bool pen_mode: false
    property bool are_color_buttons_visible: false
    property int selected_index: _index

    TouchButtonGroup{
        anchors.left: parent.left
        anchors.right: parent.right
        // anchors.top: parent.top
        // anchors.bottom: parent.bottom
        height: pen_size_slider.visible ? parent.height / 2 : parent.height
        //height: parent.height
        // width: Math.max(parent.width / 5, 150)


        id: deletebutton

        buttons: ["❌", "🖋️", "✂️", "P", ""]
        colors: ["black", root.pen_mode ? "green" : "black", "black", pen_size_slider.visible ? "green" : "black", _colors[root.selected_index]]

        visible: !are_color_buttons_visible
        onButtonClicked: function (index, name){
            if (index == 0){
                root.exitDrawModeButtonClicked();
            }
            if (index == 1){
                root.pen_mode = !root.pen_mode;
                if (root.pen_mode){
                    root.enablePenDrawModeClicked();
                }
                else{
                    root.disablePenDrawModeClicked();
                }
            }
            if (index == 2){
                root.eraserClicked();
            }
            if (index == 3){
                pen_size_slider.visible = !pen_size_slider.visible;
            }
            if (index == 4){
                root.are_color_buttons_visible= !root.are_color_buttons_visible;
            }
        }
    }

    TouchSymbolColorSelector{
        colors: _colors
        anchors.fill: parent
        visible: root.are_color_buttons_visible

        onColorClicked: function(index){
            root.changeColorClicked(index);
            root.selected_index = index;
            //root._index = index;
            root.are_color_buttons_visible = !root.are_color_buttons_visible;
        }
    }
    // TouchButtonGroup{
    //     anchors.right: parent.right
    //     anchors.left: deletebutton.right
    //     // anchors.top: parent.top
    //     // anchors.bottom: parent.bottom
    //     //height: parent.height
    //     height: pen_size_slider.visible ? parent.height / 2 : parent.height
    //     //width: 4 * parent.width / 5
    //     radio: true


    //     id: color_buttons

    //     buttons: Array(_colors.length).fill("")
    //     colors: _colors;

    //     onButtonClicked: function (index, name){
    //         root.changeColorClicked(index);
    //     }
    // }
    Slider{
        from: 0
        to: 10
        value: _pen_size
        id: pen_size_slider
        anchors.top: deletebutton.bottom
        anchors.left: parent.left
        width: parent.width
        height: parent.height / 2
        visible: false

        onValueChanged: {
                penSizeChanged(value);
                //pen_size_slider.visible = false;
        }
    }

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

