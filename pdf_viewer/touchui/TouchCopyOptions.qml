
import QtQuick 2.2
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.1

import "qrc:/pdf_viewer/touchui"

//import com.myself 1.0

Item{


    id: root
    height: 50

    signal copyPressed()
    signal scholarPressed()
    signal googlePressed()
    signal highlightPressed()

    TouchButtonGroup{
        anchors.fill: parent

        buttons: ["Copy", "Scholar", "Google", "Highlight"]
        onButtonClicked: function(index, val) {
            if (index == 0){
                root.copyPressed();
            }

            if (index == 1){
                root.scholarPressed();
            }

            if (index == 2){
                root.googlePressed();
            }

            if (index == 3){
                root.highlightPressed();
            }

        }

    }

}
