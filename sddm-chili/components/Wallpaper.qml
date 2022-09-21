/*
 *   Copyright 2018 Marian Arlt <marianarlt@icloud.com>
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
import QtGraphicalEffects 1.0

FocusScope {
    id: backgroundComponent

    property alias imageSource: backgroundImage.source
    property bool configBlur: config.blur == "true"

    Image {
        id: backgroundImage

        anchors.fill: parent
        fillMode: Image.PreserveAspectCrop

        clip: true
        focus: true
        smooth: true
    }

    RecursiveBlur {
        id: backgroundBlur

        anchors.fill: backgroundImage
        source: backgroundImage
        radius: configBlur ? config.recursiveBlurRadius : 0
        loops: configBlur ? config.recursiveBlurLoops : 0
    }

    MouseArea {
        anchors.fill: parent
        onClicked: container.focus = true
    }
}
