/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick          2.3
import QtQuick.Controls 1.2
import QtQuick.Dialogs  1.2

import QGroundControl                       1.0
import QGroundControl.Controls              1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.Palette               1.0

Column {
    id:                 tcpDualPortLinkSettings
    spacing:            ScreenTools.defaultFontPixelHeight * 0.5
    anchors.margins:    ScreenTools.defaultFontPixelWidth
    function saveSettings() {
        if(subEditConfig) {
            subEditConfig.host = hostField.text
            subEditConfig.portTo = parseInt(portToField.text)
            subEditConfig.portFrom = parseInt(portFromField.text)
        }
    }
    Row {
        spacing:        ScreenTools.defaultFontPixelWidth
        QGCLabel {
            text:       qsTr("Host Address:")
            width:      _firstColumn
            anchors.verticalCenter: parent.verticalCenter
        }
        QGCTextField {
            id:         hostField
            text:       subEditConfig && subEditConfig.linkType === LinkConfiguration.TypeTcpDualPort ? subEditConfig.host : ""
            width:      _secondColumn
            anchors.verticalCenter: parent.verticalCenter
        }
    }
    Row {
        spacing:        ScreenTools.defaultFontPixelWidth
        QGCLabel {
            text:       qsTr("TCP Port TO the vehicle:")
            width:      _firstColumn
            anchors.verticalCenter: parent.verticalCenter
        }
        QGCTextField {
            id:         portToField
            text:       subEditConfig && subEditConfig.linkType === LinkConfiguration.TypeTcpDualPort ? subEditConfig.portTo.toString() : ""
            width:      _firstColumn
            inputMethodHints: Qt.ImhFormattedNumbersOnly
            anchors.verticalCenter: parent.verticalCenter
        }
    }
    Row {
        spacing:        ScreenTools.defaultFontPixelWidth
        QGCLabel {
            text:       qsTr("TCP Port FROM the vehicle:")
            width:      _firstColumn
            anchors.verticalCenter: parent.verticalCenter
        }
        QGCTextField {
            id:         portFromField
            text:       subEditConfig && subEditConfig.linkType === LinkConfiguration.TypeTcpDualPort ? subEditConfig.portFrom.toString() : ""
            width:      _firstColumn
            inputMethodHints: Qt.ImhFormattedNumbersOnly
            anchors.verticalCenter: parent.verticalCenter
        }
    }
}
