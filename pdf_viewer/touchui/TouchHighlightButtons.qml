
import QtQuick 2.2
import QtQuick.Controls 2.15

import "qrc:/pdf_viewer/touchui"


Item{
	signal deleteButtonClicked();
	signal changeColorClicked(index: int);

	id: root

	TouchButtonGroup{
		anchors.left: parent.left
		anchors.top: parent.top
		anchors.bottom: parent.bottom
		width: parent.width / 5


		id: deletebutton

		buttons: ["delete"]
		color: "red"

		onButtonClicked: function (index, name){
			if (index == 0){
				root.deleteButtonClicked();
			}
		}
	}

	Rectangle{
		anchors.right: color_b.left
		anchors.verticalCenter: parent.verticalCenter
		anchors.margins: 10
		width: parent.width / 6
		height: width
		radius: Math.max(width,height)
		id: color_a
		color: _color_a
		MouseArea{
			anchors.fill: parent
			onClicked: {
				root.changeColorClicked(0);
			}
		}
	}
	Rectangle{
		anchors.right: color_c.left
		anchors.verticalCenter: parent.verticalCenter
		anchors.margins: 10
		width: parent.width / 6
		height: width
		radius: Math.max(width,height)
		id: color_b
		color: _color_b
		MouseArea{
			anchors.fill: parent
			onClicked: {
				root.changeColorClicked(1);
			}
		}
	}
	Rectangle{
		anchors.right: color_d.left
		anchors.verticalCenter: parent.verticalCenter
		anchors.margins: 10
		width: parent.width / 6
		height: width
		radius: Math.max(width,height)
		id: color_c
		color: _color_c
		MouseArea{
			anchors.fill: parent
			onClicked: {
				root.changeColorClicked(2);
			}
		}
	}
	Rectangle{
		anchors.verticalCenter: parent.verticalCenter
		anchors.right: parent.right
		anchors.margins: 10
		width: parent.width / 6
		height: width
		radius: Math.max(width,height)
		id: color_d
		color: _color_d
		MouseArea{
			anchors.fill: parent
			onClicked: {
				root.changeColorClicked(3);
			}
		}
	}
}

