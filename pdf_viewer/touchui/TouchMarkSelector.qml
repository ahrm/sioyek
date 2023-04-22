
import QtQuick 2.2
import QtQuick.Controls 2.15

import "qrc:/pdf_viewer/touchui"


// Rectangle{
//     color: "black"
// }

TextInput{

    signal markSelected(mark: string);

    id: root
    focus: true
	inputMethodHints: Qt.ImhSensitiveData | Qt.ImhPreferLowercase | Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText

    //buttons: ["delete"]
    //color: "red"

    onTextChanged: {
        markSelected(text);
    }
}

