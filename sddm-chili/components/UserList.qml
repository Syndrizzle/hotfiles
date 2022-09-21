/*
 *   Copyright 2018 Marian Alexander Arlt <marianarlt@icloud.com>
 *   Copyright 2014 David Edmundson <davidedmundson@kde.org>
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

ListView {
    id: view

    readonly property string selectedUser: currentItem ? currentItem.userName : ""
    readonly property int userItemWidth: root.width / 10
    readonly property int userItemHeight: avatarSize + passwordField.height / 2

    property int rootFontSize
    property string rootFontColor
    property int avatarSize

    implicitHeight: userItemHeight

    activeFocusOnTab : true

    signal userSelected;

    orientation: ListView.Horizontal
    highlightRangeMode: ListView.StrictlyEnforceRange

    preferredHighlightBegin: width / 2 - userItemWidth / 2
    preferredHighlightEnd: preferredHighlightBegin

    delegate: UserDelegate {
        
        avatarPath: model.icon || ""
        usernameFontSize : rootFontSize
        usernameFontColor: rootFontColor
        faceSize: avatarSize

        name: {
            var displayName = model.realName || model.name

            if (model.vtNumber === undefined || model.vtNumber < 0) {
                return displayName
            }

            if (!model.session) {
                return "Nobody logged in on that session", "Unused"
            }

            var location = ""

            if (model.isTty) {
                location = "User logged in on console number", "TTY %1", model.vtNumber
            } else if (model.displayNumber) {
                location = "User logged in on console (X display number)", "on TTY %1 (Display %2)", model.vtNumber, model.displayNumber
            }

            if (location) {
                return "Username (location)", "%1 (%2)", displayName, location
            }

            return displayName
        }

        userName: model.name

        width: userItemWidth
        height: userItemHeight

        constrainText: ListView.view.count > 1

        isCurrent: ListView.isCurrentItem

        onClicked: {
            ListView.view.currentIndex = index;
            ListView.view.userSelected();
        }
    }

    Keys.onEscapePressed: view.userSelected()
    Keys.onEnterPressed: view.userSelected()
    Keys.onReturnPressed: view.userSelected()
}
