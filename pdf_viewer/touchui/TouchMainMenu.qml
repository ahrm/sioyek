import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.1
import QtQuick.Controls.Material

import "qrc:/pdf_viewer/touchui"


ColumnLayout{
    spacing: 0

    signal selectTextClicked();
    signal gotoPageClicked();
    signal tocClicked();
    signal searchClicked();
    signal fullscreenClicked();
    signal bookmarksClicked();
    signal globalBookmarksClicked();
    signal highlightsClicked();
    signal globalHighlightsClicked();
    signal rulerModeClicked();
    signal lightColorschemeClicked();
    signal darkColorschemeClicked();
    signal customColorschemeClicked();
    signal openPrevDocClicked();
    signal openNewDocClicked();
    signal commandsClicked();
    signal settingsClicked();
    signal addBookmarkClicked();
    signal portalClicked();
    signal deletePortalClicked();
    signal ttsClicked();
    signal horizontalLockClicked();

    Rectangle{
        anchors.fill: parent
        z: -1
        color: "#222"
        radius: 10
    }
    Item{

        Layout.preferredHeight: Math.max(parent.height / 6, 100)
        Layout.preferredWidth: parent.width
        Column{
            anchors.fill: parent


            Text{
                id: label
                text: "Tools"
                color: "white"
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top
                anchors.margins: 10
            }

            TouchButtonGroup{
                buttons: ["qrc:/icons/text-selection.svg",
                "qrc:/icons/document-page-number.svg",
                "qrc:/icons/table-of-contents.svg",
                "qrc:/icons/search.svg",
                _fullscreen ? "qrc:/icons/fullscreen-enabled.svg" : "qrc:/icons/fullscreen.svg",
                "qrc:/icons/bookmark.svg",
                "qrc:/icons/highlight.svg",
                _ruler ? "qrc:/icons/ruler-enabled.svg" : "qrc:/icons/ruler.svg" ,
                ]

                tips: ["Select Text",
                "Goto Page",
                "Table of Contents",
                "Search",
                "Fullscreen",
                "Bookmarks",
                "Highlights",
                "Toggle Ruler Mode"]


                id: firsttools
                anchors.bottom: parent.bottom
                anchors.top: label.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: 10
                onButtonClicked: function (index, name){
                    switch (index){
                        case 0:
                            /* emit */ selectTextClicked(); break;
                        case 1:
                            /* emit */ gotoPageClicked(); break;
                        case 2:
                            /* emit */ tocClicked(); break;
                        case 3:
                            /* emit */ searchClicked(); break;
                        case 4:
                            /* emit */ fullscreenClicked(); break;
                        case 5:
                            /* emit */ bookmarksClicked(); break;
                        case 6:
                            /* emit */ highlightsClicked(); break;
                        case 7:
                            /* emit */ rulerModeClicked(); break;
                        default:
                    }
                }

            }

        }
    }

    Item{

        //Layout.preferredHeight: Math.max(parent.height / 6, 100)
        Layout.preferredHeight: firsttools.height + 20
        Layout.preferredWidth: parent.width
        Column{
            anchors.fill: parent

            TouchButtonGroup{
                buttons: ["qrc:/icons/bookmark-add.svg",
                "qrc:/icons/link.svg",
                "qrc:/icons/unlink.svg",
                _speaking ? "qrc:/icons/tts-enabled.svg" :  "qrc:/icons/tts.svg",
                _locked ? "qrc:/icons/lock-enabled.svg" :"qrc:/icons/lock.svg",
                "qrc:/icons/bookmark-g.svg",
                "qrc:/icons/highlight-g.svg",
                ]

                tips: ["Add Bookmark",
                "Portal",
                "Delete Portal",
                "Text to Speech",
                "Lock Horizontal Scroll",
                "All Bookmarks",
                "All Highlights"]


                id: secondtools
                anchors.bottom: parent.bottom
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: 10
                onButtonClicked: function (index, name){
                    switch (index){
                        case 0:
                            /* emit */ addBookmarkClicked(); break;
                        case 1:
                            /* emit */ portalClicked(); break;
                        case 2:
                            /* emit */ deletePortalClicked(); break;
                        case 3:
                            /* emit */ ttsClicked(); break;
                        case 4:
                            /* emit */ horizontalLockClicked(); break;
                        case 5:
                            /* emit */ globalBookmarksClicked(); break;
                        case 6:
                            /* emit */ globalHighlightsClicked(); break;
                        case 7:
                            /* emit */ rulerModeClicked(); break;
                        default:
                    }
                }

            }

        }
    }

    Item{

        // color: "yellow"
//        radius: 10
        // Layout.preferredHeight: 5 * parent.height / 18
        Layout.fillHeight: true
        Layout.preferredWidth: parent.width
        // height: 5 * parent.height / 18
        // anchors.left: parent.left
        // anchors.right: parent.right
        Row{
            anchors.fill: parent


            Text{
                id: label0
                text: "Theme"
                color: "white"
                //anchors.horizontalCenter: parent.horizontalCenter
                //anchors.top: parent.top
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.margins: 10
                //            color: "#00B2FF"
            }

            TouchButtonGroup{
                buttons: ["Light", "Dark", "Custom"]
                radio: true
                selectedIndex: _currentColorschemeIndex
                //anchors.bottom: parent.bottom
                //anchors.top: label0.bottom
                //anchors.left: parent.left
                //anchors.right: parent.right

                anchors.bottom: parent.bottom
                anchors.top: parent.top
                anchors.left: label0.right
                anchors.right: parent.right

                anchors.margins: 10
                onButtonClicked: function(index, name){
                    if (index == 0){
                        lightColorschemeClicked();
                    }
                    if (index == 1){
                        darkColorschemeClicked();
                    }
                    if (index == 2){
                        customColorschemeClicked();
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
        Row{
            anchors.fill: parent


            Text{
                id: label2
                color: "white"
                text: "Open"
                //anchors.horizontalCenter: parent.horizontalCenter
                //anchors.top: parent.top
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.margins: 10
                //            color: "#00B2FF"
            }

            TouchButtonGroup{
                buttons: ["Previous", "New"]
                //anchors.bottom: parent.bottom
                //anchors.top: label2.bottom
                //anchors.left: parent.left
                //anchors.right: parent.right

                anchors.bottom: parent.bottom
                anchors.top: parent.top
                anchors.left: label2.right
                anchors.right: parent.right
                anchors.margins: 10

                onButtonClicked: function (index, name){
                    if (index == 0){
                        openPrevDocClicked();
                    }
                    if (index == 1){
                        openNewDocClicked();
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
        Column{
            anchors.fill: parent


            // Text{
            //     id: label3
            //     color: "white"
            //     text: "Advanced"
            //     anchors.horizontalCenter: parent.horizontalCenter
            //     anchors.top: parent.top
            //     anchors.margins: 10
            //     //            color: "#00B2FF"
            // }

            TouchButtonGroup{
                buttons: ["Commands", "Settings"]
                anchors.bottom: parent.bottom
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: 10
                onButtonClicked: function(index, name){
                    if (index == 0){
                        commandsClicked();
                    }
                    if (index == 1){
                        settingsClicked();
                    }
                }

            }
        }
    }

}


