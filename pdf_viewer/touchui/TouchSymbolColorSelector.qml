
import QtQuick 2.2
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.1

Item{
    id: root
    property list<color> colors;

    signal colorClicked(int index)
    signal colorHeld(int index)

    Repeater{
        model: 26

        Rectangle{
            required property int index
            anchors.top: get_top_anchor()
            anchors.bottom: get_bottom_anchor()

            width: is_too_small() ? parent.width / 13 : parent.width / 26
            x: is_too_small() ?  (visible ? (index % 13) * width : root.width / 2 - width / 2) : (visible ? index * width : root.width / 2 - width / 2)
            color: root.colors[index]

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
                    root.colorClicked(index);
                }
                onPressAndHold: {
                    root.colorHeld(index);
                }
            }
        }

    }
}