/*
 *   Copyright 2018 Marian Alexander Arlt <marianarlt@icloud.com>
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
import QtQuick.Controls.Styles 1.4

ToolButton {
    id: root

    property int currentIndex: -1
    property int rootFontSize
    property string rootFontColor

    visible: menu.items.length > 1

    opacity: root.activeFocus ? 1 : 0.5

    style: ButtonStyle {
        label: Label {
            id: buttonLabel
            color: rootFontColor
            font.pointSize: rootFontSize
            renderType: Text.QtRendering
            text: instantiator.objectAt(currentIndex).text || ""
            font.underline: root.activeFocus
        }
        background: Rectangle {
            color: "transparent"
        }
    }

    Component.onCompleted: {
        currentIndex = sessionModel.lastIndex
    }

    menu: Menu {
        id: menu
        Instantiator {
            id: instantiator
            model: sessionModel
            onObjectAdded: menu.insertItem(index, object)
            onObjectRemoved: menu.removeItem( object )
            delegate: MenuItem {
                text: model.name
                onTriggered: {
                    root.currentIndex = model.index
                }
            }
        }
    }
}
