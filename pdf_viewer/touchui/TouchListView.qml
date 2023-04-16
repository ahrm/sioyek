
import QtQuick 2.2
import QtQuick.Controls 2.15


Rectangle {

//    width: 250
//    height: 400
    id: rootitem
    color: "black"
    property bool deletable: _deletable || false

    signal itemSelected(item: string, index: int)
    signal itemPressAndHold(item: string, index: int)
    signal itemDeleted(item: string, index: int)

    TextInput{
        id: query
        color: "white"
        inputMethodHints: Qt.ImhSensitiveData | Qt.ImhPreferLowercase
        focus: _focus

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
            lview.model.setFilterRegularExpression(text);
        }

		onAccepted: {
            let item = lview.model.data(lview.model.index(0, 0));
            //console.log(item);
			rootitem.itemSelected(item, 0);
		}

    }


    ListView{
//        model: MyModel {}
        anchors {
            top: query.bottom
            topMargin: 10
            left: rootitem.left
            right: rootitem.right
            bottom: rootitem.bottom
        }
        model: _model
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
            height: inner.height * 2.5
//            color: index % 2 == 0 ? "black" : "#080808"
            color: index % 2 == 0 ? "black" : "#111"
            id: bg

            Item{
                anchors.left: parent.left
                //anchors.right: parent.right
                width: inner.contentWidth
                anchors.leftMargin: 10
                anchors.verticalCenter: parent.verticalCenter
                Text {
                    id: inner
                    text: model.display
                    color: "white"
                    anchors.verticalCenter: parent.verticalCenter
                    font.pixelSize: 15
                }

            }
            Item{
                anchors.right: parent.right
                width: pagetext.contentWidth
                anchors.rightMargin: 10
                anchors.verticalCenter: parent.verticalCenter
                Text {
                    id: pagetext
                    //text: model.display
					text: lview.model.data(lview.model.index(index, 1));
                    color: "white"
                    anchors.verticalCenter: parent.verticalCenter
                    font.pixelSize: 15
                }
                visible: _model.columnCount() == 2
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
