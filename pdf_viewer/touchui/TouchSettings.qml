import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.1
import QtQuick.Controls.Material
import Qt.labs.platform
import "qrc:/pdf_viewer/touchui"



Rectangle{
    z: -1
    color: "#222"
    radius: 10

    id: root

    signal lightApplicationBackgroundClicked();
    signal darkApplicationBackgroundClicked();
    signal customApplicationBackgroundClicked();
    signal customPageTextClicked();
    signal customPageBackgroundClicked();
    signal rulerModeBoundsClicked();
    signal rulerModeColorClicked();
    signal rulerNextClicked();
    signal rulerPrevClicked();
    signal backClicked();
    signal forwardClicked();
    signal allConfigsClicked();
    signal restoreDefaultsClicked();
    signal portalClicked();
    signal saveConfigs();

    ColumnLayout{
        anchors.fill: parent
        spacing: 0


        MessageDialog {
            id: default_message
            buttons: MessageDialog.Ok | MessageDialog.Cancel
            text: "Are you sure you want to restore all settings to default?"
            onOkClicked: {
                console.log("accepted!!!!!!!!!!!!!!!!");
                root.restoreDefaultsClicked();
            }
        }

        // MessageDialog {
        // 	id: save_message
        // 	buttons: MessageDialog.Ok | MessageDialog.Cancel
        // 	text: "Persist changes?"
        //     onAccepted: {
        //         console.log("on accepted called!!!!!!!!!!!!!!!!");
        //         root.saveConfigs();
        //     }
        // }

        // Item{

        //     // color: "yellow"
        //     // radius: 10
        //     // height: parent.height / 6
        //     Layout.preferredHeight: Math.max(parent.height / 6, 100)
        //     Layout.preferredWidth: parent.width
        //     // anchors.left: parent.left
        //     // anchors.right: parent.right
        //     Column{
        //         anchors.fill: parent


        //         Text{
        //             id: label
        //             text: "Tools"
        //             color: "white"
        //             anchors.horizontalCenter: parent.horizontalCenter
        //             anchors.top: parent.top
        //             anchors.margins: 10
        //         }

        //         TouchButtonGroup{
        //             buttons: ["qrc:/icons/select_text.svg", "qrc:/icons/page.svg", "📗", "🔎", "↕", "📑", "✍", "📏"]
        //             tips: ["Select Text", "Goto Page", "Table of Contents", "Search", "Fullscreen", "Bookmarks", "Highlights", "Toggle Ruler Mode"]

        //             anchors.bottom: parent.bottom
        //             anchors.top: label.bottom
        //             anchors.left: parent.left
        //             anchors.right: parent.right
        //             anchors.margins: 10
        //             onButtonClicked: function (index, name){
        //                 switch (index){
        //                     case 0:
        //                         /* emit */ selectTextClicked(); break;
        //                     case 1:
        //                         /* emit */ gotoPageClicked(); break;
        //                     case 2:
        //                         /* emit */ tocClicked(); break;
        //                     case 3:
        //                         /* emit */ searchClicked(); break;
        //                     case 4:
        //                         /* emit */ fullscreenClicked(); break;
        //                     case 5:
        //                         /* emit */ bookmarksClicked(); break;
        //                     case 6:
        //                         /* emit */ highlightsClicked(); break;
        //                     case 7:
        //                         /* emit */ rulerModeClicked(); break;
        //                     default:
        //                 }
        //             }

        //         }
        //     }
        // }

        Item{

            // color: "yellow"
    //        radius: 10
            // Layout.preferredHeight: 5 * parent.height / 18
            Layout.fillHeight: true
            Layout.preferredWidth: parent.width
            // height: 5 * parent.height / 18
            // anchors.left: parent.left
            // anchors.right: parent.right
            Item{
                //anchors.fill: parent
                width: parent.width
                height: parent.height


                Text{
                    id: label0
                    text: "Application Background"
                    color: "white"
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: parent.top
                    anchors.margins: 10
                    //            color: "#00B2FF"
                }

                TouchButtonGroup{
                    buttons: ["Light", "Dark", "Custom"]
                    anchors.bottom: parent.bottom
                    anchors.top: label0.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.margins: 10
                    onButtonClicked: function(index, name){
                        if (index == 0){
                            lightApplicationBackgroundClicked();
                        }
                        if (index == 1){
                            darkApplicationBackgroundClicked();
                        }
                        if (index == 2){
                            customApplicationBackgroundClicked();
                        }
                    }

                }
            }
        }

        Item{

            // color: "yellow"
    //        radius: 10
            // height: 5 * parent.height / 18
            // Layout.preferredHeight: 5 * parent.height / 18
            Layout.fillHeight: true
            Layout.preferredWidth: parent.width
            // anchors.left: parent.left
            // anchors.right: parent.right
            Item{
                //anchors.fill: parent
                width: parent.width
                height: parent.height



                Text{
                    id: label2
                    color: "white"
                    text: "Custom Color Mode"
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: parent.top
                    anchors.margins: 10
                    //            color: "#00B2FF"
                }

                TouchButtonGroup{
                    buttons: ["Page Text", "Page Background"]
                    anchors.bottom: parent.bottom
                    anchors.top: label2.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.margins: 10

                    onButtonClicked: function (index, name){
                        if (index == 0){
                            customPageTextClicked();
                        }
                        if (index == 1){
                            customPageBackgroundClicked();
                        }
                    }

                }
            }
        }

        Item{

            // color: "yellow"
    //        radius: 10
            // height: 5 * parent.height / 18
            // Layout.preferredHeight: 5 * parent.height / 18
            Layout.fillHeight: true
            Layout.preferredWidth: parent.width
            // anchors.left: parent.left
            // anchors.right: parent.right
            Item{
                anchors.fill: parent
                width: parent.width
                height: parent.height


                Text{
                    id: label3
                    color: "white"
                    text: "Ruler Mode"
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: parent.top
                    anchors.margins: 10
                    //            color: "#00B2FF"
                }

                TouchButtonGroup{
                    buttons: ["Bounds", "Color"]
                    anchors.bottom: parent.bottom
                    anchors.top: label3.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.margins: 10
                    onButtonClicked: function(index, name){
                        if (index == 0){
                            rulerModeBoundsClicked();
                        }
                        if (index == 1){
                            rulerModeColorClicked();
                        }
                    }

                }
            }
        }

        Item{
            // Layout.preferredHeight: 5 * parent.height / 18
            Layout.fillHeight: true
            Layout.preferredWidth: parent.width
            // anchors.left: parent.left
            // anchors.right: parent.right
            Item{
                anchors.fill: parent
                width: parent.width
                height: parent.height


                Text{
                    id: label4
                    color: "white"
                    text: "Shortcut Locations"
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: parent.top
                    anchors.margins: 10
                    //            color: "#00B2FF"
                }

                TouchButtonGroup{
                    buttons: ["Ruler ↓", "Ruler ↑", "Back", "Forward", "Portal"]
                    anchors.bottom: parent.bottom
                    anchors.top: label4.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.margins: 10
                    onButtonClicked: function(index, name){
                        if (index == 0){
                            rulerNextClicked();
                        }
                        if (index == 1){
                            rulerPrevClicked();
                        }
                        if (index == 2){
                            backClicked();
                        }
                        if (index == 3){
                            forwardClicked();
                        }
                        if (index == 4){
                            portalClicked();
                        }
                    }

                }
            }
        }

        Item{
            // Layout.preferredHeight: 5 * parent.height / 18
            Layout.fillHeight: true
            Layout.preferredWidth: parent.width
            // anchors.left: parent.left
            // anchors.right: parent.right
            Item{
                //anchors.fill: parent
                width: parent.width
                height: parent.height


                Text{
                    id: label5
                    color: "white"
                    text: "Miscellaneous"
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: parent.top
                    anchors.margins: 10
                    //            color: "#00B2FF"
                }

                TouchButtonGroup{
                    buttons: ["All Configs", "Restore Defaults"]
                    anchors.bottom: parent.bottom
                    anchors.top: label5.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.margins: 10
                    onButtonClicked: function(index, name){
                        if (index == 0){
                            allConfigsClicked();
                        }
                        if (index == 1){
                            default_message.open();
                            //root.restoreDefaultsClicked();
                        }
                    }

                }
            }
        }

    }


}
