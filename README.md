<p align="center"> <img src="assets/fvwm_hotfiles.gif" align="center" width="600"> </p>

<p align="center"><a href="#installation"><img alt="picture 1" src="https://i.imgur.com/v820gOe.gif" width="150" /></a>&nbsp;&nbsp;&nbsp;<a href="#gallery"><img alt="picture 3" src="https://i.imgur.com/dB9MC33.gif" width="150" /></a>&nbsp;&nbsp;&nbsp;<a href="#post-installation-notes"><img alt="picture 4" src="https://i.imgur.com/VovEDmf.gif" width="150"/></a>&nbsp;&nbsp;&nbsp;<a href="#credits"><img alt="picture 5" src="https://i.imgur.com/gJHFRGk.gif" width="150" /></a></p>  

<p align="center"><img src="https://img.shields.io/github/issues/syndrizzle/hotfiles?color=171b20&label=Issues%20%20&logo=gnubash&labelColor=e05f65&logoColor=ffffff">&nbsp;&nbsp;&nbsp;<img src="https://img.shields.io/github/forks/syndrizzle/hotfiles?color=171b20&label=Forks%20%20&logo=git&labelColor=f1cf8a&logoColor=ffffff">&nbsp;&nbsp;&nbsp;<img src="https://img.shields.io/github/stars/syndrizzle/hotfiles?color=171b20&label=Stargazers&logo=github&labelColor=70a5eb">&nbsp;&nbsp;&nbsp;<img src="https://img.shields.io/badge/-Works on my machine-%2378dba9?style=flat&logo=linux&labelColor=171b20&logoColor=ffffff"></p>

<p align="center"><b>Thanks for the visit!</b><br><br>
Welcome to the <b>Hotfiles ️‍🔥</b> repository! This branch contains configuration files for the FVWM, or F Virtual Window Manager, carefully designed with usability and functionality in mind, while being light on resources!
<br>(It uses ~800MB of RAM)</p>

### 3. 💮 The Flawless FVWM 💮

#### About:
* **FVWM** as the window manager.
* **Decay** as the color scheme.
* **Kitty** as the terminal emulator.
* **Dunst (Fork)** as the notification daemon.
* **EWW** as the widgets and the bottom panel.
* **DockbarX** as the dock.
* **Picom** as the compositor.
* **Rofi** as the application menu.
* **Albert Launcher** as the universal search.
* **JGMenu** as the desktop menu.
* **SLiM** as the desktop manager.
* **i3lock** as the lock screen.
* **Some love** as the essence!

⚠️ **NOTE!!!!!** This configuration was made for my PC, so things here might not work on your PC, in that case, please try if you can fix that up, or you can open an issue for help :) This was made for a screen having a resolution of **1920x1080** and, on a **Laptop** with **120** as the screen DPI (Dots Per Inch).

---

## Installation:
Below are the installation steps to install these dotfiles. Click on a step to expand it.

<details>
<summary><b> 1. Installing the dependencies:</b></summary>
<br>
<details>
<summary><b>Installation on ArchLinux</b></summary>

It is recommended to do this on a fresh installed system, since that way you can grab the most out of it. If you are unsure, you can always install the dependencies manually. And if something breaks while not being on a fresh install, please do.  
Do note that this process might take time depending on your hardware, so why not do some exercise while you wait? ❤️🏋️

Before we begin the installation, we need to have three things ready:

1. We need to create the user directories, this makes the future installation easier.
2. We need to install the main tools required to build and install stuff.
3. To make the process easier, we can enable [Chaotic AUR](https://chaotic.cx), which has some precompiled binaries of certin programs we need, which narrows down the installation time.

<b>1. Creating User Directories:</b>  
First we need to install the `xdg-user-dirs` package:
```bash
sudo pacman -Syyy xdg-user-dirs
```
Then, to create directories, such as `Desktop`, `Documents`, `Downloads`, we can run the below command:
```bash
xdg-user-dirs-update
```
Now check the directories, if they are created using `ls`, if they are not, please run the above command again.

<b>2. Installing the main tools:</b>  
We can install the main tools `base-devel`, which is a package group by executing the below command:
```bash
sudo pacman -S base-devel
```

<b>3. Enabling Chaotic AUR:</b>  
To Enable the Chaotic AUR, you can follow the steps on their website https://chaotic.cx.

Now, we can move on to installing the packages. Make sure you have your favourite AUR Helper ready, in this case we will be using [paru](https://aur.archlinux.org/packages/paru/), but you can use any of the AUR Helpers available like [yay](https://aur.archlinux.org/packages/yay/).

To save time, we can run the below command to install the packages in one go:
```bash
paru -S nitrogen noto-fonts-emoji xorg-xinit slim fish starship papirus-icon-theme xfce4-power-manager xfce4-settings jgmenu thunar thunar-archive-plugin thunar-volman xarchiver unzip unrar rofi picom kitty dockbarx gtk2 gtk3 xorg python git make nerd-fonts-jetbrains-mono playerctl libwnck3 python-pip gtk-engine-murrine appmenu-gtk-module libappindicator-gtk3 libappindicator-gtk2 light pamixer wmctrl bc curl jq maim acpi python-praw tint2 pavucontrol albert redshift qt5ct lightly-git mate-polkit python-wand conky-lua fvwm-git npm yaru-sound-theme zsh mpv
```

We are not done yet! We need to install the remaining packages, which unfortunately can't be installed from the AUR Helper.

But first, we need the `git`! If you haven't already installed, git, please do, its awesome :) (We need it for the stuff below)
```bash
sudo pacman -S git
```


<b>1. Elkowar's Wacky Widgets (EWW)</b>  
Elkowar's wacky widgets are the main widgets that we are gonna use in our system. It is a very essential dependency that you need.
First you need the nightly version of rust and also GTK3. A speedy way would be to directly install the binary package of rust nightly from the AUR using your favorite AUR helper:
```bash
paru -S rust-nightly-bin gtk3
```
Then we just need to run a few commands assuming you have `git` installed:
```bash
cd ~/Downloads
git clone https://github.com/elkowar/eww.git
cd eww
cargo build --release -j $(nproc)
cd target/release
sudo mv eww /usr/bin/eww
```
That installs eww to our root filesystem, which is then sourced from the `$PATH`.

<b>2. Dunst</b>  
We are not using the dunst which ships with the distro's packages, instead, there is a fork which has some very nice additions to the main dunst look, the images are now rounded to match the UI, and also the Notifications have a nice gradient look!<br>Thanks to [k-veroony](https://github.com/k-vernooy/dunst)!

First, install the dependencies, most of them should be already present if not all, due to that big command we ran above!
```bash
paru -S systemd gdk-pixbuf2 pango libxss libxinerama libxrandr wayland wayland-protocols libnotify
```
Now, clone the repo and compile dunst:
```bash
cd ~/Downloads
git clone https://github.com/k-vernooy/dunst -b progress-styling
cd dunst
make
sudo make install
```

That's it! We have successfully installed all the dependencies!
</details>
<details>
<summary><b>Installation on Ubuntu</b></summary>
Do it yourself, or why not stop using Ubuntu?
</details>
</details>
<details>
<summary><b>2. Installing the GTK Theme:</b></summary>
Since we are using Decay, as our color scheme, we need to install the Decay GTK Theme, to match the overall look of our system.

```bash
git clone https://github.com/decaycs/gtk3 decay-gtk3
cd decay-gtk3/decay
sudo npm install -g sass
make && sudo make install
```
</details>
<details>
<summary><b>3. Installing the Dotfiles:</b></summary>
This is the last and the most awaited step!

Clone the dotfiles:
```bash
cd ~/Downloads
git clone https://github.com/syndrizzle/hotfiles.git -b fvwm
cd hotfiles
cp -r .config .cache .fvwm .icons .local .scripts .wallpapers ~/
cp .xinitrc .Xresources .gtkrc-2.0 .api_keys ~/
```

Move the `slim.conf` in the `/etc` directory and also move stuff from the `usr/` directory:
```bash
sudo cp etc/slim.conf /etc/slim.conf
sudo cp -r usr/* /usr/
```

And you are done! Enjoy!
</details>

---

#### Miscllaneous Stuff:
<details>
<summary><b>Install the Spotify Theme:</b></summary>
Since we copied the dotfiles, we can apply the spicetify theme now. First, install spicetify using:

```bash
curl -fsSL https://raw.githubusercontent.com/spicetify/spicetify-cli/master/install.sh | sh
curl -fsSL https://raw.githubusercontent.com/spicetify/spicetify-marketplace/main/resources/install.sh | sh
```
Then, we need to give read and write access to our spotify folder for modifications:

```bash
sudo chmod a+wr /opt/spotify
sudo chmod a+wr /opt/spotify/Apps -R
```

After that we just need to run:

```bash
bash
spicetify config current_theme Ziro
spicetify config color_scheme decay
spicetify config extensions adblock.js
spicetify backup apply
```

This would install the spicetify theme to your Spotify.
</details>

<details>
<summary><b>Visual Studio Code Theme:</b></summary>  
Follow the instructions on the <a href="https://github.com/decaycs/vscode">decay for vscode</a> readme to install Decay for Visual Studio Code.
</details>

---

## Gallery:

<img alt="picture 1" src="https://i.imgur.com/7lG8QRE.jpg" />  
<p align="center">The Desktop</p>

<img alt="picture 3" src="https://i.imgur.com/lIxriMC.jpg" />  
<p align="center">The Control Center</p>

<img alt="picture 4" src="https://i.imgur.com/jnxgr4j.jpg" />  
<p align="center">Spicetify Theme</p>

<img alt="picture 5" src="https://i.imgur.com/jKU9bbg.jpg" />  
<p align="center">SLiM Theme</p>

<img alt="picture 7" src="https://i.imgur.com/0VUgsPk.jpg" />  
<p align="center">The Lock Screen</p>

---

## Post Installation Notes:

<details>
<summary><b>Get github notifications, unread emails and your reddit karma:</b></summary>
If you open the control center, in the bottom, there are several tiles which display the notifications, unread emails and your reddit karma, along with the weather! In order to get that, you just need to edit one file, it is located in your home directory, hidden as `.api_keys`.

But first, you need to get the credentials, like the API keys, accounts, passwords.

**1. Reddit**  
For this, we need the following stuff:
* The reddit client ID
* The reddit client secret
* The reddit username (Yours)
* The reddit password
* The reddit account of which you want to get the karma

You can obtain the first two from the [reddit developer portal](https://www.reddit.com/prefs/apps/).
The rest depend on you and your reddit account.

**2. GMail**  
For this, we need the following stuff:
* The email of your google account
* The application password needed to authenticate your google account  
  
**THIS APPLICATION PASSWORD IS NOT YOUR GOOGLE ACCOUNT PASSWORD, GOOGLE DISCONTINUED THE ABILITY TO USE GOOGLE PASSWORDS TO AUTHENTICATE WITH IMAP THIS APRIL (2022)**  
Use the official page [here](https://support.google.com/mail/answer/185833?hl=en-GB) to get the application password.  
Copy it and paste it in the `.api_keys` file.

You will also need to enable IMAP in your google account, just head over to [your gmail settings](https://mail.google.com/mail/#settings/fwdandpop) and click on Enable IMAP, then save the settings.

**3. GitHub**  
For this, we need the following stuff:
* The github personal access token
* The github username (Yours)

[Check the official docs to know how to obtain a personal access token here.](https://docs.github.com/en/authentication/keeping-your-account-and-data-secure/creating-a-personal-access-token)

**4. Weather**  
For this, we need the following stuff:
* The weather API key, obtainable from OpenWeatherMap
* THe city for which you want the weather data to be shown
* The units (Metric or Imperial)
  
You can get an openweathermap key by [signing up](https://home.openweathermap.org/users/sign_up) for openweathermap, then visit the API Keys section to generate one.
</details>

<details>
<summary><b>Enable QT5 apps theming:</b></summary>
If you are not getting universal look for your QT5 apps, follow these steps:
We need to edit the `/etc/environment` to make qt5ct (the app we installed that we will use to theme qt5 apps) the default for qt5 apps.  

/etc/environment:
```bash
#
# This file is parsed by pam_env module
#
# Syntax: simple "KEY=VAL" pairs on separate lines
#
QT_QPA_PLATFORMTHEME=qt5ct
```
Then, you gotta reboot for the changes to take effect.  
After rebooting, open qt5ct from the app drawer or the terminal and select `Lightly` as the style, along with `Decay` as the color scheme. Click Apply.
</details>
<details>
<summary><b>Keybindings:</b></summary>
Following are the keybindings of the setup:
<br>
<ul>
    <li><kbd>Ctrl+L</kbd>: Lock the screen</li>
    <li><kbd>Ctrl+B</kbd>: Open the Browser (Firefox)</li>
    <li><kbd>Ctrl+F</kbd>: Open the File Manager (Thunar)</li>
    <li><kbd>Super+Enter</kbd>: Open the Terminal</li>
    <li><kbd>Super+T</kbd>: Toggle maximizing a window.</li>
    <li><kbd>Super+Space</kbd>: Open global search (Albert)</li>
    <li><kbd>Super+[1,2,3,4]</kbd>: Switch to the respective desktops, 1,2,3,4.</li>
    <li><kbd>Alt+F4</kbd>: Quit an application.</li>
<br>
The general keybindings like volume, brightness, music/media keys also work as intended, tho it may vary from keyboard to keyboard.
</details>

## Support
You can always show your support towards these rices by donating me, as this takes a lot of time and hardwork, thank you for using these dotfiles!

<b>Ko-Fi:</b> <br>
[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/A0A8DKCLY) <br>

<b>Librepay:</b> <br>
<a href="https://liberapay.com/syndrizzle/donate"><img alt="Donate using Liberapay" src="https://liberapay.com/assets/widgets/donate.svg"></a>

--- 
## Credits

Thanks to:
* [dharmx](https://github.com/dharmx)
* [janleigh](https://github.com/janleigh)
* [Axarva](https://github.com/Axarva)
* [adi1090x](https://github.com/adi1090x)
* [decaycs](https://github.com/decaycs)

The icons used in this dotfiles are from the [Phosphor Icons](https://phosphoricons.com/) project.
The external icons are from Icons8,FlatIcons and IconScout, and are used under the [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/) license.

The overall look of the project was adapted from concepts around the internet, along with chromeOS.

<3
