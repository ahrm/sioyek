
//import QtQuick 2.15
import QtQuick
import QtQuick.Controls 2.15
import MySortFilterProxyModel 1.0


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
            tree_view.model.setFilterRegularExpression(text);
            //tree_view.model.setFilterCustom(text);
            //_model.setFilterCustom(text);
        }

        onAccepted: {
            let item = tree_view.model.data(lview.model.index(0, 0));
            //console.log(item);
            rootitem.itemSelected(item, 0);
        }

    }


    TreeView{
        anchors {
            top: query.bottom
            topMargin: 10
            left: rootitem.left
            right: rootitem.right
            bottom: rootitem.bottom
        }
        model: _model
        //horizontalScrollBarPolicy: Qt.ScrollBarAlwaysOff


        id: tree_view
        clip: true
        flickableDirection: Flickable.VerticalFlick

        delegate: Rectangle {
            // visible: column == 0
//            anchors {
//                left: parent ? parent.left : undefined
//                right: parent ? parent.right : undefined
//            }

//            height: rootitem.height

            required property TreeView treeView
            required property bool isTreeNode
            required property bool expanded
            required property int hasChildren
            required property int depth

            implicitWidth: column == 0 ? rootitem.width - 60 : 60

            //anchors.left: column == 0 ? parent.left : undefined
            //anchors.right: column == 1 ? parent.right : undefined

            // implicitWidth: tree_view.width
            implicitHeight: inner.height * 2.5
//            color: index % 2 == 0 ? "black" : "#080808"
            color: row % 2 == 0 ? "black" : "#111"
            id: bg

            function getPrefix(){
                if (column == 1){
                    return "";
                }
                let prefix = "";
                for (let i = 0; i < depth; i++){
                    prefix += "\t";
                }

                if (hasChildren && expanded){
                return prefix +  "-";
                }
                if (hasChildren && (!expanded)){
                return prefix + ">";
                }
                return prefix;

            }

            TapHandler {
                onSingleTapped: {
                    let ind = tree_view.modelIndex(row, column);
                    console.log("----");
                    console.log(row);
                    console.log(ind.row);
                    console.log(ind.column);
                    console.log(ind.depth);
                    //console.log(index);
                    //console.log(model.index);
                    //console.log(Object.keys(model));
                    //treeView.toggleExpanded(row)
                }
                onLongPressed: {
                    treeView.toggleExpanded(row)
                }
            }

            Item{
                anchors.left: column == 0 ? parent.left : undefined
                anchors.right: column == 0 ? undefined : parent.right
                //anchors.right: parent.right
                width: inner.contentWidth
                anchors.leftMargin: 10
                anchors.verticalCenter: parent.verticalCenter

                Text {
                    id: inner
                    text: bg.getPrefix() + model.display
                    color: "white"
                    anchors.verticalCenter: parent.verticalCenter
                    font.pixelSize: 15
                }

            }

            // Item{
            //     anchors.right: parent.right
            //     //anchors.right: parent.right
            //     width: pagelabel.contentWidth
            //     anchors.rightMargin: 10
            //     anchors.verticalCenter: parent.verticalCenter

            //     Text {
            //         id: pagelabel
            //         //text: model.display
            //         text: _model.data(_model.index(model.index, 1))
            //         color: "white"
            //         anchors.verticalCenter: parent.verticalCenter
            //         font.pixelSize: 15
            //     }

            // }
        //    Item{
        //        anchors.right: parent.right
        //        width: pagetext.contentWidth
        //        anchors.rightMargin: 10
        //        anchors.verticalCenter: parent.verticalCenter
        //        Text {
        //            id: pagetext
        //            //text: model.display
        //            text: _model.data(_model.index(index, 1));
        //            color: "white"
        //            anchors.verticalCenter: parent.verticalCenter
        //            font.pixelSize: 15
        //        }
        //        visible: _model.columnCount() == 2
        //    }

            // MouseArea {
            //     anchors.fill: parent

            //     onPressAndHold: function(event) {
            //         if (rootitem.deletable){
            //             deletebutton.visible = true;
            //         }
            //         else{
            //             /* emit */ itemPressAndHold(model.display, index);
            //         }
            //     }
            //     onClicked: {
            //         console.log("???");
            //         lview.currentIndex = index;
            //         /* emit */ itemSelected(model.display, index);

            //     }

            // }
        }


    }
//    TreeView{
////        model: MyModel {}
//        anchors {
//            top: query.bottom
//            topMargin: 10
//            left: rootitem.left
//            right: rootitem.right
//            bottom: rootitem.bottom
//        }
//        model: _model
//        id: lview
//        clip: true
//        //anchors.fill: parent


//        // displaced: Transition{
//        //     PropertyAction {
//        //         properties: "opacity, scale"
//        //         value: 1
//        //     }

//        //     NumberAnimation{
//        //         properties: "x, y"
//        //         duration: 50
//        //     }
//        // }


//        delegate: Rectangle {
//            anchors {
//                left: parent ? parent.left : undefined
//                right: parent ? parent.right : undefined
//            }
//            height: inner.height * 2.5
////            color: index % 2 == 0 ? "black" : "#080808"
//            color: index % 2 == 0 ? "black" : "#111"
//            id: bg

//            Item{
//                anchors.left: parent.left
//                //anchors.right: parent.right
//                width: inner.contentWidth
//                anchors.leftMargin: 10
//                anchors.verticalCenter: parent.verticalCenter
//                Text {
//                    id: inner
//                    text: model.display
//                    color: "white"
//                    anchors.verticalCenter: parent.verticalCenter
//                    font.pixelSize: 15
//                }

//            }
//            Item{
//                anchors.right: parent.right
//                width: pagetext.contentWidth
//                anchors.rightMargin: 10
//                anchors.verticalCenter: parent.verticalCenter
//                Text {
//                    id: pagetext
//                    //text: model.display
//                    text: _model.data(_model.index(index, 1));
//                    color: "white"
//                    anchors.verticalCenter: parent.verticalCenter
//                    font.pixelSize: 15
//                }
//                visible: _model.columnCount() == 2
//            }

//            MouseArea {
//                anchors.fill: parent

//                onPressAndHold: function(event) {
//                    if (rootitem.deletable){
//						deletebutton.visible = true;
//                    }
//                    else{
//						/* emit */ itemPressAndHold(model.display, index);
//					}
//                }
//                onClicked: {
//                    lview.currentIndex = index;
//                    /* emit */ itemSelected(model.display, index);

//                }

//            }
//        }


//    }
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
