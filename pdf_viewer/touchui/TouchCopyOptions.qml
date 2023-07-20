
import QtQuick 2.2
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.1

import "qrc:/pdf_viewer/touchui"

//import com.myself 1.0

Item{


    id: root
    height: 50

    signal copyPressed()
    signal searchPressed()
    signal scholarPressed()
    signal googlePressed()
    signal highlightPressed()
    signal downloadPressed()

    TouchButtonGroup{
        anchors.fill: parent

        buttons: [
        "qrc:/icons/copy.svg",
        "qrc:/icons/search.svg",
        "qrc:/icons/google-scholar.svg",
        "qrc:/icons/google.svg",
        "qrc:/icons/paper-download.svg",
        "qrc:/icons/highlight.svg",
        ]

        tips: ["Copy", "Search in PDF", "Search in Google Scholar", "Search in Google", "Download", "Highlight"]
        onButtonClicked: function(index, val) {
            if (index == 0){
                root.copyPressed();
            }

            if (index == 1){
                root.searchPressed();
            }

            if (index == 2){
                root.scholarPressed();
            }

            if (index == 3){
                root.googlePressed();
            }

            if (index == 4){
                root.downloadPressed();
            }
            if (index == 5){
                root.highlightPressed();
            }

        }

    }

}
