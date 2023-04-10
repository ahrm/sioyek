
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

    TouchButtonGroup{
        anchors.fill: parent

        buttons: ["Copy", "Search", "Scholar", "Google", "Highlight"]
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
                root.highlightPressed();
            }

        }

    }

}
