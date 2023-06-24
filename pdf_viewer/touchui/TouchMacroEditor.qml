
import QtQuick 2.2
import QtQuick.Controls 2.15

import "qrc:/pdf_viewer/touchui"

Rectangle {
    color: "#444"
    radius: 10
    id: root
    property list<string> items: _macro.split(';')
    property list<string> command_options: ["move_down", "move_up", "debug"]

    signal confirmed(val: string)
    signal canceled()

		function getNameFromMacroText(txt){
			let parts = txt.split(']');
			if (parts.length == 1){
				return txt;
			}
			else{
				return parts[1];
			}
		}

        function getMacroModifierString(txt){
            let parts = txt.split(']');
            if (parts.length == 2){
                return parts[0].slice(1, parts[0].length);
            }
            else{
                return "";
            }
        }
		function getModesFromMacroText(txt){
			let modes = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
			let modes_lower = "rxhqedpost";
			let modes_upper = "RXHQEDPOST";
			let index_map = {};

			for (let i = 0; i < modes_lower.length; i++){
				index_map[modes_lower[i]] = i;
				index_map[modes_upper[i]] = i;
			}
			let parts = txt.split(']');
			if (parts.length == 2){
				let mode_string = parts[0].slice(1, parts[0].length);
				for (let i = 0; i < mode_string.length; i++){
					let mode_char = mode_string[i];
					let index = index_map[mode_char];
					if (mode_char.toLowerCase() == mode_char){
						modes[index] = 1;
					}
					else{
						modes[index] = 2;
					}
				}
			}

			return modes;

		}

    function getMacroString(){
        return root.items.join(";");
    }

    TextEdit{
       
       anchors.top: parent.top
       id: macro_text
       width: root.width
		height: root.width / 20
        readOnly: true
        selectByMouse: true
        color: "white"
        text:  getMacroString()
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
    }

    TouchListView{
        width: root.width
        height: root.height
        anchors.left: root.left
        anchors.top: root.top
        z: 1
        visible: false
        root_model: _commands_model
        id: command_selector
        // model: root.command_options

    }

    TouchButtonGroup{
        anchors.top: macro_text.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        width: root.width / 2
        height: root.width / 20
        buttons: ["OK", "Cancel", "+"]
        id: add_button
        onButtonClicked: function(ind, val){
            if (ind == 0){
                root.confirmed(getMacroString());
            }
            else if (ind == 1){
                root.canceled();
            }
            else if (ind == 2){
                console.log("what");
                root.items.push("");
            }
        }
    }
    // Rectangle {
    //     anchors.top: macro_text.bottom
	// 	width: root.width / 20
	// 	height: root.width / 20
    //     radius: root.width / 20

    //     color: "red"
    //     //anchors.top: parent.top
    //     anchors.horizontalCenter: parent.horizontalCenter
    //     id: add_button

    //     Text{
    //         anchors.centerIn: parent
    //         text: "+"
    //         font.pointSize: 20
    //     }

    //     MouseArea{
    //         id: ma
    //         anchors.fill: parent
    //         onClicked: {
	// 			root.items.push("");
    //         }
    //     }
    // }

    Flickable{
		anchors.top: add_button.bottom
		anchors.left: parent.left
		anchors.right: parent.right
		anchors.bottom: parent.bottom
        anchors.topMargin: 10

        contentWidth: column_container.width
        contentHeight: column_container.height
        clip: true

        Column{
            id: column_container
            spacing: 10

            Repeater{
                model: root.items

                Rectangle {
                    id: delegate_id
                    width: root.width
                    height: 100
                    color: index % 2 == 0 ? "#222" : "#333"
                    Rectangle{
                        id: text_background
                        color: "white"
                        anchors.verticalCenter: parent.verticalCenter
                        height: 50
                        width: parent.width / 2
                        anchors.left: parent.left
                        anchors.leftMargin: 10
                        radius: 3

                        Text {
                            anchors.fill: parent
                            id: command_name
                            text:  root.getNameFromMacroText(root.items[index]).length > 0 ? root.getNameFromMacroText(root.items[index]) : "[SELECT COMMAND]"

                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: Text.AlignHCenter

                            // onAccepted: function(){
                            //     root.items[index] = command_name.text;
                            // }
                        }
                        MouseArea{
                            anchors.fill: parent

                            onClicked: function () {
                                command_selector.visible = true;
                                command_selector.itemSelected.connect(function(val){
                                    let macro_string = getMacroModifierString(root.items[index]);
                                    command_selector.visible = false;
                                    if (macro_string.length > 0){
                                        root.items[index] = "[" + macro_string + "]" + val;
                                    }
                                    else{
                                        root.items[index] = val;
                                    }
                                });
                            }
                        }

                        // TextInput {
                        //     anchors.fill: parent
                        //     verticalAlignment: Text.AlignVCenter
                        //     id: command_name
                        //     text:  root.getNameFromMacroText(root.items[index])
                        //     //color: "white"
                        //     // onTextChanged: function(){
                        //     //     root.items[index] = command_name.text;
                        //     // }

                        //     onAccepted: function(){
                        //         root.items[index] = command_name.text;
                        //     }
                        //     //onEditingFinished: function (){
                        //         //root.items[index] = command_name.text;
                        //         //command_name.focus = true;
                        //         //return false;
                        //     //}
                        // }
                    }

                TouchButtonGroup{
                    // anchors.top: parent.top
                    // anchors.bottom: parent.bottom
                    height: parent.height / 2
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: delete_button.left
                    anchors.left: text_background.right
                    width: parent.width / 10
                    // roundRight: false
                    id: mode_buttons
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10


                    buttons: ["r", "x", "h", "q", "e", "d", "p", "o", "s", "t"]
                    tips: ["Ruler", "Synxtex", "Highlight on select", "Freehand drawing", "Pen freehand drawing", "Mouse drag", "Presentation", "Overview", "Searching", "Text selected"]
                    //modeList: [0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
                    modeList: root.getModesFromMacroText(root.items[index])

                    modal: true
                    onButtonClicked: function(ind, val) {
                        let modes = mode_buttons.modeList;
                        let mode_string = "";
                        for (let i = 0; i< modes.length; i++){
                            if (modes[i] == 1){
                                mode_string += mode_buttons.buttons[i];
                            }
                            else if (modes[i] == 2){
                                mode_string += mode_buttons.buttons[i].toUpperCase();
                            }
                        }

                        if (mode_string.length > 0){
                            root.items[index] = "[" + mode_string + "]" + root.getNameFromMacroText(root.items[index]);
                        }
                        else{
                            root.items[index] = root.getNameFromMacroText(root.items[index]);
                        }

                    }
                }
                TouchButtonGroup{
                    id: delete_button
                    // anchors.top: parent.top
                    // anchors.bottom: parent.bottom
                    height: parent.height / 2
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    width: parent.width / 10
                    anchors.rightMargin: 10

                    buttons: ["Delete"]
                    color: "red"
                    // roundLeft: false
                    // roundRight: false
                    onButtonClicked: function(ind, val) {
                        if (ind == 0){
                            root.items.splice(index, 1);
                        }
                    }
                }


                }
            }
        }
    }
    // ListView{

	// 	anchors.top: add_button.bottom
	// 	anchors.left: parent.left
	// 	anchors.right: parent.right
	// 	anchors.bottom: parent.bottom
    //     model: root.items
    //     clip: true
	// 	id: list_view

	// 	function getNameFromMacroText(txt){
	// 		let parts = txt.split(']');
	// 		if (parts.length == 1){
	// 			return txt;
	// 		}
	// 		else{
	// 			return parts[1];
	// 		}
	// 	}

	// 	function getModesFromMacroText(txt){
	// 		let modes = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
	// 		let modes_lower = "rxhqedpost";
	// 		let modes_upper = "RXHQEDPOST";
	// 		let index_map = {};

	// 		for (let i = 0; i < modes_lower.length; i++){
	// 			index_map[modes_lower[i]] = i;
	// 			index_map[modes_upper[i]] = i;
	// 		}
	// 		let parts = txt.split(']');
	// 		if (parts.length == 2){
	// 			let mode_string = parts[0].slice(1, parts[0].length);
	// 			console.log(mode_string);
	// 			for (let i = 0; i < mode_string.length; i++){
	// 				let mode_char = mode_string[i];
	// 				let index = index_map[mode_char];
	// 				if (mode_char.toLowerCase() == mode_char){
	// 					modes[index] = 1;
	// 				}
	// 				else{
	// 					modes[index] = 2;
	// 				}
	// 			}
	// 		}

	// 		return modes;

	// 	}

    //     delegate: Rectangle {
    //         id: delegate_id
    //         width: root.width
    //         height: 100
    //         color: index % 2 == 0 ? "#222" : "#333"
    //         Rectangle{
	// 			id: text_background
	// 			color: "white"
	// 			anchors.verticalCenter: parent.verticalCenter
	// 			height: 50
	// 			width: parent.width / 2
	// 			anchors.left: parent.left

	// 			TextInput {
	// 				anchors.fill: parent
	// 				verticalAlignment: Text.AlignVCenter
	// 				id: command_name
	// 				text:  list_view.getNameFromMacroText(root.items[index])
	// 				//color: "white"
	// 				onAccepted: function(){
	// 					root.items[index] = command_name.text;
	// 				}
	// 				//onEditingFinished: function (){
	// 					//root.items[index] = command_name.text;
	// 					//command_name.focus = true;
	// 					//return false;
	// 				//}
	// 			}
    //         }

	// 	TouchButtonGroup{
    //         anchors.top: parent.top
    //         anchors.bottom: parent.bottom
    //         anchors.right: delete_button.left
    //         anchors.left: text_background.right
    //         width: parent.width / 10
	// 		id: mode_buttons


	// 		buttons: ["r", "x", "h", "q", "e", "d", "p", "o", "s", "t"]
    //         tips: ["Ruler", "Synxtex", "Highlight on select", "Freehand drawing", "Pen freehand drawing", "Mouse drag", "Presentation", "Overview", "Searching", "Text selected"]
    //         //modeList: [0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
    //         modeList: list_view.getModesFromMacroText(root.items[index])

    //         modal: true
	// 		onButtonClicked: function(ind, val) {
	// 			let modes = mode_buttons.modeList;
	// 			let mode_string = "";
	// 			for (let i = 0; i< modes.length; i++){
	// 				if (modes[i] == 1){
	// 					mode_string += mode_buttons.buttons[i];
	// 				}
	// 				else if (modes[i] == 2){
	// 					mode_string += mode_buttons.buttons[i].toUpperCase();
	// 				}
	// 			}

	// 			if (mode_string.length > 0){
	// 				root.items[index] = "[" + mode_string + "]" + list_view.getNameFromMacroText(root.items[index]);
	// 			}
	// 			else{
	// 				root.items[index] = list_view.getNameFromMacroText(root.items[index]);
	// 			}

	// 		}
	// 	}
	// 	TouchButtonGroup{
    //         id: delete_button
    //         anchors.top: parent.top
    //         anchors.bottom: parent.bottom
    //         anchors.right: parent.right
    //         width: parent.width / 10

	// 		buttons: ["Delete"]
	// 		onButtonClicked: function(ind, val) {
	// 			if (ind == 0){
    //                 root.items.splice(index, 1);
    //             }
	// 		}
	// 	}


    //     }

    // }


}

