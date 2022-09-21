/*
 *   Copyright 2018 Marian Arlt <marianarlt@icloud.com>
 *   Copyright 2016 David Edmundson <davidedmundson@kde.org>
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
import QtQuick.Layouts 1.1
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4

LoginFormLayout {

	property string lastUserName
	property bool passwordFieldOutlined: config.PasswordFieldOutlined == "true"
    property int inputSpacing: 8

    signal loginRequest(string username, string password)

    function startLogin() {
        var username = userList.selectedUser
        var password = passwordField.text
        loginRequest(username, password);
    }

    RowLayout {

        Layout.leftMargin: loginButton.width + inputSpacing * 2
        Layout.minimumWidth: passwordField.width + loginButton.width + inputSpacing * 2

        TextField {
            id: passwordField


        	placeholderText: textConstants.password
            echoMode: TextInput.Password
            onAccepted: startLogin()
        	focus: true

            font.pointSize: usernameFontSize * 0.9
            implicitWidth: root.width / 5
            implicitHeight: usernameFontSize * 2.75
            opacity: 0.5

            style: TextFieldStyle {
                textColor: passwordFieldOutlined ? "white" : "black"
                placeholderTextColor: passwordFieldOutlined ? "white" : "black"
                background: Rectangle {
                    radius: 10
                    border.color: "white"
                    border.width: 1
                    color: passwordFieldOutlined ? "transparent" : "white"
                }
            }

        	Keys.onEscapePressed: {
                loginFormStack.currentItem.forceActiveFocus();
            }

            Keys.onPressed: {
                if (event.key == Qt.Key_Left && !text) {
                    userList.decrementCurrentIndex();
                    event.accepted = true
                }
                if (event.key == Qt.Key_Right && !text) {
                    userList.incrementCurrentIndex();
                    event.accepted = true
                }
            }

            Keys.onReleased: {
                if (loginButton.opacity == 0 && length > 0) {
                    showLoginButton.start()
                }
                if (loginButton.opacity > 0 && length == 0) {
                    hideLoginButton.start()
                }
            }

            Connections {
                target: sddm
                onLoginFailed: {
                    passwordField.selectAll()
                    passwordField.forceActiveFocus()
                }
            }
        }
        
        Image {
            id: loginButton

            Layout.leftMargin: inputSpacing

            source: "../assets/login.svgz"
            sourceSize: Qt.size(passwordField.height, passwordField.height)
            smooth: true
            opacity: 0
            visible: opacity > 0

            MouseArea {
                anchors.fill: parent
                onClicked: startLogin();
            }

            PropertyAnimation {
                id: showLoginButton
                target: loginButton
                properties: "opacity"
                to: 0.75
                duration: 100
            }
            
            PropertyAnimation {
                id: hideLoginButton
                target: loginButton
                properties: "opacity"
                to: 0
                duration: 80
            }
        }
        
    }

}
