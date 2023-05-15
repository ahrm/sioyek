
import QtQuick 2.2
import QtQuick.Controls 2.15

import "qrc:/pdf_viewer/touchui"


Rectangle{
	signal exitDrawModeButtonClicked();
	signal changeColorClicked(index: int);
	signal enablePenDrawModeClicked();
	signal disablePenDrawModeClicked();
	signal eraserClicked();

	id: root
	//color: "black"
	color: "#00ffffff"
	radius: 5
	property bool pen_mode: false

	TouchButtonGroup{
		anchors.left: parent.left
		anchors.top: parent.top
		anchors.bottom: parent.bottom
		width: parent.width / 5


		id: deletebutton

		buttons: ["🗑️", "🖋️", "✂️"]
		colors: ["black", root.pen_mode ? "green" : "black", "black"]

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
		}
	}

	TouchButtonGroup{
		anchors.right: parent.right
		anchors.top: parent.top
		anchors.bottom: parent.bottom
		width: 4 * parent.width / 5
		radio: true


		id: color_buttons

		buttons: Array(_colors.length).fill("")
		colors: _colors;

		onButtonClicked: function (index, name){
			root.changeColorClicked(index);
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

