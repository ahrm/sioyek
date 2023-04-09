import QtQuick 2.2
import QtQuick.Controls 2.15
import Qt.labs.platform 1.1

// Row{

//     TextEdit{
//         id: filtertext
//         anchors.top: parent.top
//         anchors.left: parent.left
//         anchors.right: parent.right
//         onTextChanged: {
//             console.log("what/");
//             lview.model.setFilterRegularExpression(text);
//         }
//     }
//     ListView{
//         id: lview
//         anchors.top: filtertext.bottom
//         anchors.bottom: parent.bottom
//         anchors.left: parent.left
//         anchors.right: parent.right
//         model: _model

//         delegate: Rectangle{


//                         width: innertext.width
//                         height: innertext.height
//                         Text{
//                             id: innertext
//                             text: _model.data(_model.index(index, 1))
//                         }
//         }

        
//     }
// }

Rectangle {


    id: rootitem
    color: "black"
    property bool deletable: _deletable || false

    signal itemSelected(item: string, index: int)
    signal floatConfigChanged(configName: string, newConfigValue: real);
    signal textConfigChanged(configName: string, newConfigValue: string);
    signal intConfigChanged(configName: string, newConfigValue: int);
    signal boolConfigChanged(configName: string, newConfigValue: bool);
    signal color3ConfigChanged(configName: string, r: real, g: real, b: real);
    signal color4ConfigChanged(configName: string, r: real, g: real, b: real, a: real);
    signal onSetConfigPressed(configName: string);
    signal onSaveButtonClicked();
    // signal itemPressAndHold(item: string, index: int)
    // signal itemDeleted(item: string, index: int)

    Row{
        id: queryrow
        anchors {
            top: rootitem.top
            left: rootitem.left
            right: rootitem.right
            leftMargin: 10
            topMargin: 10
        }
        height: query.height
        TextEdit{
            id: query
            color: "white"
            inputMethodHints: Qt.ImhSensitiveData | Qt.ImhPreferLowercase

            anchors {
                left: parent.left
                top: parent.top
                right: savebutton.left
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

        }
        Button{
            id: savebutton
            anchors{
                right: parent.right
                top: parent.top
            }
            anchors.verticalCenter: parent.verticalCenter
            text: "Save Changes"

            onClicked:{
                /* emit */ rootitem.onSaveButtonClicked();
            }
        }
    }


    ListView{
//        model: MyModel {}
        model: _model
        id: lview
        clip: true

        anchors {
            top: queryrow.bottom
            topMargin: 10
            left: rootitem.left
            right: rootitem.right
            bottom: rootitem.bottom
        }

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
            height: holded ?  inner.height * 5 : inner.height * 2.5
//            color: index % 2 == 0 ? "black" : "#080808"
            color: index % 2 == 0 ? "black" : "#111"
            property bool holded: false
            id: bg
            property string type: _model.data(_model.index(index, 0))
            property string name: _model.data(_model.index(index, 1))
            property var value: _model.data(_model.index(index, 2))

            Item{
                anchors.left: parent.left
                anchors.leftMargin: 10
                anchors.verticalCenter: parent.verticalCenter
                Text {
                    id: inner
                    text: bg.name
                    color: "white"
                    anchors.verticalCenter: parent.verticalCenter
                    font.pixelSize: 15
                }

            }

            Item{
                z: 2
                anchors.right: parent.right
                anchors.rightMargin: 10
//                anchors.verticalCenter: parent.verticalCenter
                anchors.top: parent.top
                anchors.bottom: parent.bottom

                ColorDialog{
                    id: colordialog
                    options: ColorDialog.ShowAlphaChannel
                    onAccepted: function(){
                        colorrect.color = color;

                         if (bg.type == 'color3'){
                            /* emit */ color3ConfigChanged(bg.name, color.r, color.g, color.b);
                         }
                         if (bg.type == 'color4'){
                            /* emit */ color4ConfigChanged(bg.name, color.r, color.g, color.b, color.a);
                         }


                    }
                }

                Rectangle{
                    id: colorrect
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.right: parent.right
                    anchors.topMargin: 10
                    anchors.bottomMargin: 10
//                    anchors.verticalCenter: parent.verticalCenter
                    width: 100
//                    height: 100
                    color: bg.value
                    visible: (bg.type == 'color3' || bg.type == 'color4')

                    MouseArea{
                        anchors.fill: parent

                        onClicked: {
                            console.log("handle color config");
                            if (bg.type == 'color3'){
                                colordialog.options = 0;
                            }
                            else{

                                colordialog.options = ColorDialog.ShowAlphaChannel;
                            }
                            colordialog.color = colorrect.color;

                            colordialog.open();
                        }
                    }
                }

                CheckBox{
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.right: parent.right
                    anchors.margins: 10
                    checked: bg.value
                    visible: (bg.type == 'bool')

                    onCheckedChanged: {
                        if (visible){
							/* emit */ boolConfigChanged(bg.name, checked);
                        }
                    }

                }

//                    Slider{

//                        id: innerSlider
//                        anchors.top: parent.top
//                        anchors.bottom: parent.bottom
//                        anchors.right: parent.right
//                        value: bg.value[0]
//                        from: bg.value[1]
//                        to: bg.value[2]

//                        visible: bg.type == 'float'

//                        onValueChanged: {
//                            if (visible){
//								/* emit */ floatConfigChanged(bg.name, value);
//                            }
//                        }

//                    }

                    
                    // TextEdit{

                    //     id: textEdit
                    //     anchors.top: parent.top
                    //     anchors.bottom: parent.bottom
                    //     anchors.right: parent.right
                    //     text: bg.value
                    //     width: 100

                    //     visible: bg.type == 'string'


                    // }
//                    Slider{

//                        id: intSlider
//                        anchors.top: parent.top
//                        anchors.bottom: parent.bottom
//                        anchors.right: parent.right
//                        value: bg.value[0]
//                        from: bg.value[1]
//                        to: bg.value[2]

//                        visible: bg.type == 'int'

//                        onValueChanged: {
//                            if (visible){
//								/* emit */ intConfigChanged(bg.name, Math.round(value));
//                            }
//                        }

//                    }
//                Text {
//                    id: sliderValue
//                    text: bg.type == 'int' ? Math.round(intSlider.value) : innerSlider.value.toFixed(2)
//                    color: "white"
//                    anchors.verticalCenter: parent.verticalCenter
//                    anchors.right: innerSlider.left
//                    font.pixelSize: 15
//                    visible: bg.type == 'float' || bg.type == 'int'
//                }



//                ScrollView{

//                    anchors.verticalCenter: parent.verticalCenter
//                    anchors.right: parent.right
//                    width: 100

//                    TextArea {
//                        id: innerValue
//                        text: bg.value
//                        // color: "white"
//                        font.pixelSize: 15
//                        visible: (bg.type == 'string')
//                        onTextChanged: {
//                            if (bg.type == 'string'){
//                                /* emit */ textConfigChanged(bg.name, text);
//                            }
//                        }
//                    }
//                    visible: (bg.type == 'string')
//                }
                Button{
                    function getValue(){
                        if (bg.type == 'string'){

                            if (bg.value.length == 0){
                                return "Set";
                            }

                            if (bg.value.length < 8){
                                return bg.value;
                            }
                            else{
                                return bg.value.slice(0, 5) + '...';
                            }

                        }
                        if (bg.type == 'float') return bg.value[0].toFixed(2);
                        if (bg.type == 'int') return bg.value[0];
                        return "Set";
                    }

                    text: "" + getValue()
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    visible: !(bg.type == 'bool' || bg.type == 'color3' || bg.type == 'color4')
                    onClicked: {
                        /* emit */ onSetConfigPressed(bg.name);
                    }
                }

            }

//            }

            Behavior on height{

                NumberAnimation {
                    duration: 200
                    easing.type: Easing.OutExpo
                }
            }

            MouseArea {
                anchors.fill: parent

                onPressAndHold: function(event) {
                    bg.holded = true;

                    if (rootitem.deletable){
						deletebutton.visible = true;
                    }
                    else{
						/* emit */ itemPressAndHold(model.display, index);
					}
                }

                onReleased: {
                    holded = false;
                }

                onClicked: {
//                    console.log("clicked");
//                    console.log(innerSlider.value);
                    console.log(bg.value);
                    lview.currentIndex = index;
                    /* emit */ itemSelected(model.display, index);

                }

            }
        }


    }
}

