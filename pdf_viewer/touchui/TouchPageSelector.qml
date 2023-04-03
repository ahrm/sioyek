
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.1
import QtQuick.Controls.Material

Column{

    signal pageSelected(page: int);


    Slider{
        from: _from
        to: _to
        value: _value
        id: slider
        property int prevValue: -1;

        background: Rectangle {
            x: slider.leftPadding
            y: slider.topPadding + slider.availableHeight / 2 - height / 2
            implicitWidth: 200
            implicitHeight: 4
            width: slider.availableWidth
            height: implicitHeight
            radius: 2
            color: "#FF8400"

            Rectangle {
                width: slider.visualPosition * parent.width
                height: parent.height
                color: "#00B2FF"
                radius: 2
            }

        }


        handle: Rectangle{
            x: slider.leftPadding + slider.visualPosition * (slider.availableWidth - width)
            y: slider.topPadding + slider.availableHeight / 2 - height / 2

            implicitWidth: 20
            implicitHeight: 20
            radius: 20
            id: handle

            Rectangle{

                color: "black"
//                width: 40
                height: textid.height + 10
                width: textid.width + 10
                radius: height
                y: -40
                anchors.horizontalCenter: handle.horizontalCenter
                TextInput{
                    color: "white"
                    id: textid
                    anchors.centerIn: parent
//                    anchors.fill:  parent
                    text: Math.round(slider.value)
                    horizontalAlignment: Text.AlignHCenter
                    inputMethodHints: Qt.ImhDigitsOnly

                    onAccepted: {
                        slider.value = text;
                        focus = false;
                    }


                    MouseArea {
                        anchors.fill: parent
//                        propagateComposedEvents: true
                        onClicked: {
//                            console.log("somethiong");
//                            mouse.accepted = false;
                            textid.text = "";
                            textid.focus = true;

                        }
                    }

                }
            }


            Behavior on y{
                NumberAnimation {
                    duration: 200
                    easing.type: Easing.OutExpo
                }
            }

            color: "black"

        }


        width: parent.width
        height: parent.height / 2

        onValueChanged: {
            if (prevValue != -1){
                console.log("selected");
                textid.text = Math.round(value);
                /* emit */ pageSelected(Math.round(value));
            }
            prevValue = Math.round(value);

        }
    }
}
