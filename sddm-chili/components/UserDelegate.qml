/*
 *   Copyright 2018 Marian Alexander Arlt <marianarlt@icloud.com>
 *   Copyright 2014 David Edmundson <davidedmundson@kde.org>
 *   Copyright 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 3 or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

import QtQuick 2.2
import QtQuick.Controls 1.4
import QtGraphicalEffects 1.0

Item {
    id: wrapper

    property bool isCurrent: true
    property bool constrainText: true
    property string name
    property string userName
    property string avatarPath
    property string iconSource
    property int usernameFontSize
    property string usernameFontColor
    
    property real faceSize
    
    readonly property var m: model
    
    signal clicked()

    opacity: isCurrent ? 1.0 : 0.3

    Behavior on opacity {
        OpacityAnimator {
            duration: 150
        }
    }

    Item {
        id: imageSource
        width: faceSize
        height: faceSize
        anchors.horizontalCenter: parent.horizontalCenter

        // Image takes priority, taking a full path to a file, if that doesn't exist we show an icon
        Image {
            id: face
            source: wrapper.avatarPath
            sourceSize: Qt.size(faceSize, faceSize)
            anchors.fill: parent
            fillMode: Image.PreserveAspectCrop
            smooth: true
            visible: false
        }
        Image {
            id: mask
            source: "../assets/mask.svgz"
            sourceSize: Qt.size(faceSize, faceSize)
            smooth: true
        }
        OpacityMask {
            anchors.fill: face
            source: face
            maskSource: mask
            cached: true
        }
    }

    Label {
        id: usernameLabel

        color: usernameFontColor
        font.capitalization: Font.Capitalize
        font.pointSize: usernameFontSize * 1.2
        renderType: Text.QtRendering
        anchors.top: imageSource.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: usernameLabel.height / 1.2
        text: wrapper.name
        horizontalAlignment: Text.AlignHCenter
        // Make an indication that this has active focus, this only happens when reached with keyboard navigation
        font.underline: wrapper.activeFocus
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        onClicked: wrapper.clicked();
    }

    Accessible.name: name
    Accessible.role: Accessible.Button
    function accessiblePressAction() { wrapper.clicked() }
}
