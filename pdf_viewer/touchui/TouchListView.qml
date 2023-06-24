
import QtQuick 2.2
import QtQuick.Controls 2.15


Rectangle {

//    width: 250
//    height: 400
    id: rootitem
    color: "black"
    property bool deletable: _deletable || false
    property var root_model: _model

    signal itemSelected(item: string, index: int)
    signal itemPressAndHold(item: string, index: int)
    signal itemDeleted(item: string, index: int)

    TextInput{
        id: query
        color: "white"
        inputMethodHints: Qt.ImhSensitiveData | Qt.ImhPreferLowercase
        focus: _focus
        activeFocusOnTab: true

        anchors {
            top: rootitem.top
            left: rootitem.left
            right: rootitem.right
            leftMargin: 10
            topMargin: 10
        }

        font.pixelSize: 15

        Text { // palceholder text
            text: "Search"
            color: "#888"
            visible: !query.text
            font.pixelSize: 15
        }


        onTextChanged: {
            //lview.model.setFilterRegularExpression(text);
            lview.model.setFilterCustom(text);
            //rootitem.root_model.setFilterRegularExpression(text);
        }

		onAccepted: {
            let item = lview.model.data(lview.model.index(0, 0));
            //console.log(item);
			rootitem.itemSelected(item, 0);
		}

    }


    ListView{
//        model: MyModel {}
        Component.onCompleted: {
            positionViewAtIndex(_selected_index, ListView.Beginning);
        }
        anchors {
            top: query.bottom
            topMargin: 10
            left: rootitem.left
            right: rootitem.right
            bottom: rootitem.bottom
        }
        //model: _model
        model: rootitem.root_model
        id: lview
        clip: true
        //anchors.fill: parent


        displaced: Transition{
            PropertyAction {
                properties: "opacity, scale"
                value: 1
            }

            NumberAnimation{
                properties: "x, y"
                duration: 50
            }
        }


        delegate: Rectangle {
            anchors {
                left: parent ? parent.left : undefined
                right: parent ? parent.right : undefined
            }
            height: Math.max(inner.height * 2.5, inner2.height * 1.5)
//            color: index % 2 == 0 ? "black" : "#080808"
            color: lview.model.mapToSource(lview.model.index(index, 0)).row == _selected_index ? "#444": (index % 2 == 0 ? "black" : "#111")
            //color: (index % 2 == 0 ? "black" : "#111")
            id: bg

            Item{
                anchors.left: parent.left
                anchors.right: right_label.left
                //anchors.right: parent.right
                anchors.leftMargin: 10
                anchors.verticalCenter: parent.verticalCenter
                Item{
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    width: rootitem.root_model.columnCount() == 3 ? parent.width / 2 : parent.width
                    id: inner_container

					Text {
						id: inner
						text: model.display
						color: "white"
						anchors.verticalCenter: parent.verticalCenter
						anchors.left: parent.left
						anchors.right: parent.right
						font.pixelSize: 15
						wrapMode: Text.Wrap
					}
                }
                Item{
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.left: inner_container.right
                    width: rootitem.root_model.columnCount() == 3 ? parent.width / 2 : parent.width
					visible: rootitem.root_model.columnCount() == 3

					Text {
						id: inner2
						text: model.display.slice(0, 0) + lview.model.data(lview.model.index(index, 1)) || "";
						color: "white"
						anchors.verticalCenter: parent.verticalCenter
						anchors.left: parent.left
						anchors.right: parent.right
						font.pixelSize: 15
						wrapMode: Text.Wrap
					}
                }

            }
            Item{
                anchors.right: parent.right
                width: Math.min(pagetext.contentWidth, parent.width / 2)
                anchors.rightMargin: 10
                anchors.verticalCenter: parent.verticalCenter
                id: right_label
                Text {
                    id: pagetext

                    // model.display.slice(0, 0) is a hack to get qml to redraw this widget
                    // when model changes there has to be a better way to do this
					text: model.display.slice(0, 0) + lview.model.data(lview.model.index(index, rootitem.root_model.columnCount()-1)) || "";
                    color: "white"
                    anchors.verticalCenter: parent.verticalCenter
                    font.pixelSize: 15
                }
                visible: rootitem.root_model.columnCount() >= 2
            }

			Button{
                anchors.right: parent.right
                anchors.top:  parent.top
                anchors.bottom: parent.bottom
                anchors.margins: 10
                z: 10

				id: deletebutton
				text: "Delete"
                visible: false

                onClicked: {
                    /* emit */ itemDeleted(model.display, index);
                    lview.model.removeRows(index, 1);
                }

			}

            MouseArea {
                anchors.fill: parent

                onPressAndHold: function(event) {
                    if (rootitem.deletable){
						deletebutton.visible = true;
                    }
                    else{
						/* emit */ itemPressAndHold(model.display, index);
					}
                }
                onClicked: {
                    lview.currentIndex = index;
                    /* emit */ itemSelected(model.display, index);

                }

            }
        }


    }
}

//Rectangle {
////    anchors.fill: parent
////    height: 300
////    width: parent.width
////    width: 100
////    height: 100

////    implicitWidth: 100
////    implicitHeight: 100
//    //    anchors.fill: parent
//    color: "#00000000"
//    //    flags:  Qt.WA_TranslucentBackground | Qt.WA_AlwaysStackOnTop
//    //    flags:  Qt.WA_AlwaysStackOnTop
//    id: root

//    signal valueSelected(val: int)

//    Slider{
//        from: _from
//        to: _to
//        id: my_slider
//        value: _initialValue

//        anchors {
//            left: parent.left
//            right: parent.right
//            verticalCenter: parent.verticalCenter
//        }



//    }
//    Button{
//        text: "Confirm"
//        anchors {
//            horizontalCenter: parent.horizontalCenter
//            top: my_slider.bottom
//        }

//        onClicked: {
//            root.valueSelected(my_slider.value);
//            //                console.log("something has happened!");
//        }
//    }

//}


//Slider{
//    width: 300
//    height: 300

//    from: _from
//    to: _to
//    id: my_slider
//    value: _initialValue

//    signal valueSelected(val: int)

////    anchors {
////        left: parent.left
////        right: parent.right
////        verticalCenter: parent.verticalCenter
////    }


//    Button{
//        text: "Confirm"
//        anchors {
//            horizontalCenter: parent.horizontalCenter
//            top: my_slider.bottom
//        }

//        onClicked: {
//            my_slider.valueSelected(my_slider.value);
//            //                console.log("something has happened!");
//        }
//    }

//}
