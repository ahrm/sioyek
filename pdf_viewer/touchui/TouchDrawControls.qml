
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
    signal screenshotClicked();
    signal toggleScratchpadClicked();
    signal saveScratchpadClicked();
    signal loadScratchpadClicked();
    signal moveClicked();

    function on_scratchpad_mode_change(mode){
        is_scratchpad = mode;
    }

    id: root
    //color: "black"
    color: "#00ffffff"
    radius: 5
    property bool pen_mode: false
    property bool are_color_buttons_visible: false
    property bool is_size_slider_visible: false
    property int selected_index: _index
    property bool is_scratchpad: false

    TouchButtonGroup{
        anchors.left: parent.left
        anchors.right: parent.right
        height: pen_size_slider.visible ? parent.height / 2 : parent.height

        visible: !is_scratchpad && (!are_color_buttons_visible) && (!is_size_slider_visible)
        id: non_scratchpad_buttons

        buttons: [
            "qrc:/icons/close.svg",
            root.pen_mode ?  "qrc:/icons/finger-index.svg" : "qrc:/icons/pen.svg",
            "qrc:/icons/move.svg",
            "qrc:/icons/erase.svg",
            "qrc:/icons/adjust.svg",
            "qrc:/icons/screenshot.svg",
            "qrc:/icons/swap.svg",
            ""
        ]
        tips: ["Close", "Toggle pen mode", "Move", "Erase", "Adjust size", "Copy screenshot to scratchpad","Toggle scratchpad" ,"Color"]
        colors: ["black", "black", "black", "black", "black", "black","black", _colors[root.selected_index]]

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
                root.moveClicked();
            }
            if (index == 3){
                root.eraserClicked();
            }
            if (index == 4){
                root.is_size_slider_visible = true;
                // pen_size_slider.visible = !pen_size_slider.visible;
            }
            if (index == 5){
                root.screenshotClicked();
            }
            if (index == 6){
                root.toggleScratchpadClicked();
            }
            if (index == 7){
                root.are_color_buttons_visible= !root.are_color_buttons_visible;
            }
        }
    }

    TouchButtonGroup{
        anchors.left: parent.left
        anchors.right: parent.right
        height: pen_size_slider.visible ? parent.height / 2 : parent.height

        visible: is_scratchpad && (!are_color_buttons_visible) && (!is_size_slider_visible)
        id: scratchpad_buttons

        buttons: [
            "qrc:/icons/close.svg",
            root.pen_mode ?  "qrc:/icons/finger-index.svg" : "qrc:/icons/pen.svg",
            "qrc:/icons/move.svg",
            "qrc:/icons/erase.svg",
            "qrc:/icons/adjust.svg",
            "qrc:/icons/screenshot.svg",
            "qrc:/icons/save.svg",
            "qrc:/icons/load.svg",
            "qrc:/icons/swap.svg",
            ""
        ]
        tips: ["Close", "Toggle pen mode", "Move", "Erase", "Adjust size", "Copy screenshot to scratchpad","Save scratchpad", "Load scratchpad", "Toggle scratchpad" ,"Color"]
        colors: ["black", "black", "black", "black", "black", "black","black", "black", "black", _colors[root.selected_index]]

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
                root.moveClicked();
            }
            if (index == 3){
                root.eraserClicked();
            }
            if (index == 4){
                root.is_size_slider_visible = true;
                // pen_size_slider.visible = !pen_size_slider.visible;
            }
            if (index == 5){
                root.screenshotClicked();
            }
            if (index == 6){
                root.saveScratchpadClicked();
            }
            if (index == 7){
                root.loadScratchpadClicked();
            }
            if (index == 8){
                root.toggleScratchpadClicked();
            }
            if (index == 9){
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
    Slider{
        from: 0
        to: 10
        value: _pen_size
        id: pen_size_slider
        anchors.top: non_scratchpad_buttons.bottom
        anchors.left: parent.left
        width: parent.width
        height: parent.height / 2
        visible: root.is_size_slider_visible
        property bool is_pressed: false

        onValueChanged: {
                penSizeChanged(value);
                //pen_size_slider.visible = false;
        }

        onPressedChanged: function(val){
            is_pressed = !is_pressed;
            if (!is_pressed){
                root.is_size_slider_visible = false;
            }
        }
    }
}

