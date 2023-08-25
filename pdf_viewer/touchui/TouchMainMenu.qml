import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.1
import QtQuick.Controls.Material

import "qrc:/pdf_viewer/touchui"


Rectangle{

    z: -1
    color: "#222"
    radius: 10

    signal selectTextClicked();
    signal gotoPageClicked();
    signal tocClicked();
    signal searchClicked();
    signal hintClicked();
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
    signal fitToPageWidthClicked();
    signal drawingModeButtonClicked();
    signal downloadPaperClicked();

    ColumnLayout{
        spacing: 0
        anchors.fill: parent


        Item{

            Layout.preferredHeight: Math.max(parent.height / 6, 100)
            Layout.preferredWidth: parent.width
            Item{
                //anchors.fill: parent
                width: parent.width
                height: parent.height

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
                    "qrc:/icons/question.svg",
                    _fullscreen ? "qrc:/icons/fullscreen-enabled.svg" : "qrc:/icons/fullscreen.svg",
                    "qrc:/icons/bookmark.svg",
                    "qrc:/icons/highlight.svg",
                    _ruler ? "qrc:/icons/ruler-enabled.svg" : "qrc:/icons/ruler.svg" ,
                    ]

                    tips: ["Select Text",
                    "Goto Page",
                    "Table of Contents",
                    "Search",
                    "Show Hints",
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
                                /* emit */ hintClicked(); break;
                            case 5:
                                /* emit */ fullscreenClicked(); break;
                            case 6:
                                /* emit */ bookmarksClicked(); break;
                            case 7:
                                /* emit */ highlightsClicked(); break;
                            case 8:
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
            Item{
                //anchors.fill: parent
                width: parent.width
                height: parent.height

                TouchButtonGroup{
                    buttons: ["qrc:/icons/bookmark-add.svg",
                    _portaling ? "qrc:/icons/link-enabled.svg" :  "qrc:/icons/link.svg",
                    "qrc:/icons/unlink.svg",
                    _speaking ? "qrc:/icons/tts-enabled.svg" :  "qrc:/icons/tts.svg",
                    "qrc:/icons/draw.svg",
                    _locked ? "qrc:/icons/lock-enabled.svg" :"qrc:/icons/lock.svg",
                    "qrc:/icons/bookmark-g.svg",
                    "qrc:/icons/highlight-g.svg",
                    _fit ? "qrc:/icons/fit-horizontal-enabled.svg" :  "qrc:/icons/fit-horizontal.svg",
                    "qrc:/icons/paper-download.svg"
                    ]

                    tips: ["Add Bookmark",
                    "Portal",
                    "Delete Portal",
                    "Text to Speech",
                    "Darwing Mode",
                    "Lock Horizontal Scroll",
                    "All Bookmarks",
                    "All Highlights",
                    "Fit to Page Width",
                    "Download Paper"
                    ]


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
                                /* emit */ drawingModeButtonClicked(); break;
                            case 5:
                                /* emit */ horizontalLockClicked(); break;
                            case 6:
                                /* emit */ globalBookmarksClicked(); break;
                            case 7:
                                /* emit */ globalHighlightsClicked(); break;
                            case 8:
                                /* emit */ fitToPageWidthClicked(); break;
                            case 9:
                                /* emit */ downloadPaperClicked(); break;
                            default:
                        }
                    }

                }

            }
        }

        Item{

            Layout.fillHeight: true
            Layout.preferredWidth: parent.width
            Row{
                width: parent.width
                height: parent.height

                spacing: 10
                leftPadding: 10
                rightPadding: 10

                Text{
                    id: label0
                    text: "Theme"
                    color: "white"
                    anchors.verticalCenter: parent.verticalCenter
                }

                TouchButtonGroup{
                    buttons: ["Light", "Dark", "Custom"]
                    radio: true
                    selectedIndex: _currentColorschemeIndex


                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width - label2.width - 35
                    height: parent.height - 20

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

            Layout.fillHeight: true
            Layout.preferredWidth: parent.width

            Row{
                width: parent.width
                height: parent.height
                spacing: 10
                leftPadding: 10
                rightPadding: 10

                Text{
                    id: label2
                    color: "white"
                    text: "Open"
                    anchors.verticalCenter: parent.verticalCenter
                }

                TouchButtonGroup{
                    buttons: ["Previous", "New"]

                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width - label2.width - 30
                    height: parent.height - 20

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

            Layout.fillHeight: true
            Layout.preferredWidth: parent.width

            Item{
                width: parent.width
                height: parent.height



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


}
