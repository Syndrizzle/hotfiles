<p align="center">
  <img src="https://i.imgur.com/EW3xu1H.png">

<h1 align="center">🔥 Hotfiles 🔥</h1>
<p align="center">
	<a href="https://github.com/syndrizzle/hotfiles/stargazers">
		<img alt="Stargazers" src="https://img.shields.io/github/stars/syndrizzle/hotfiles?style=for-the-badge&logo=starship&color=C9CBFF&logoColor=D9E0EE&labelColor=302D41"></a>&nbsp;&nbsp;
	<a href="https://github.com/syndrizzle/hotfiles/forks">
		<img alt="Forks" src="https://img.shields.io/github/forks/syndrizzle/hotfiles.svg?style=for-the-badge&logo=github&color=F2CDCD&logoColor=D9E0EE&labelColor=302D41"/></a>&nbsp;&nbsp;
	<a href="https://github.com/syndrizzle/hotfiles/issues">
		<img alt="Issues" src="https://img.shields.io/github/issues/syndrizzle/hotfiles?style=for-the-badge&logo=gitbook&color=B5E8E0&logoColor=D9E0EE&labelColor=302D41"></a>
</p>
### Thanks for visiting!

Welcome to my GitHub repository of personal dotfiles! 😃 Here you can find configuration files for various desktops, tho this is still a Work-In-Progress thing.
<p align="center"><a href="#installation"><img alt="" title="" src="https://i.imgur.com/FV7KiO6.png" width="250" height="75"></a>     <a href="#gallery"><img alt="" title="" src="https://i.imgur.com/IqBRz0o.png" width="250" height="75"></a>      <a href="#tips-and-tricks"><img alt="" title="" src="https://i.imgur.com/kF5y3hr.png" width="250" height="75"></a>     <a href="#credits"><img alt="" title="" src="https://i.imgur.com/QsHiEjr.png" width="250" height="75"></a></p>

#### 1\. 🪱 The Wabbling Worm 🪱

### About:
*   **Alacritty** as the terminal.
*   **Catppuccin** as the color scheme.
*   **WormWM** as the window manager.
*   **Picom** as the compositor.
*   **EWW** as the widgets \[Dashboard, Bar and Sidebar\]
*   **Rofi** as the application launcher.
*   **SLiM** as the Display Manager.
*   **Dunst** as the notification daemon.
*   **Conky** as the desktop eyecandy.
*   **jgmenu** as the desktop root menu.
*   **AltTab** as the window switcher.

⚠️ **NOTE!!!!!** This configuration was made for my PC, so things here might not work on your PC, in that case, please try if you can fix that up, or you can open an issue for help :) This was made for a **1920x1080** screen and on a **Laptop** with **96** dpi.

### 🔔 Figma Files:

**There are some figma files that you might need while configuring the dotfiles.**
*   The lock screen (Since it is just glued, lol.) -> [Figma](https://www.figma.com/file/i2PIFn8hfkX71CsnMtViWv/SLiM-login-template)
*   The Conky background -> [Figma](https://www.figma.com/file/T4pKJ1IyOj2M1csbP7WEIU/Conky-Alterf-Catppuccin)
*   The Lock Screen Background -> [Figma](https://www.figma.com/file/RqQuZn54bFBGgUQm9Gs1D4/Lock-Screen-bg)

Installation
=====

### 1\. Packages:

I will try my best to make the installation procedure easier. However, if you need help, feel free to DM me on Discord: `Syndrizzle#3826` or you can open an issue on github. 😊

For this setup, you need the following packages:

* [WormWM](https://github.com/codic12/worm) \[The main window manager :p\]
* [EWW](https://github.com/elkowar/eww) \[😮 For the beautiful things!\]
* [Conky](https://github.com/brndnmtthws/conky) \[For desktop widget\]
* [Dunst](https://github.com/dunst-project/dunst) \[For Notifications\]
* [tint2](https://gitlab.com/o9000/tint2) \[For System Tray\]
* [jgmenu](https://github.com/johanmalm/jgmenu) \[For root desktop menu\] For jgmenu, pleae edit the `~/.config/jgmenu/prepend.csv` to change the username of the home directory from `syndrizzle` to your linux username, since jgmenu only recognizes absolute paths for some reason.
* [rofi](https://github.com/davatorium/rofi) \[For the application launcher\]
* [Alacritty](https://github.com/alacritty/alacritty) \[🚀 Blazing fast terminal emulator\]
* [Nitrogen](https://github.com/l3ib/nitrogen) \[Wallpaper app\]
* [Picom (Jonaburg Fix)](https://github.com/Arian8j2/picom-jonaburg-fix) \[The compositor! But with fixes to support shadows with blur ✨\]
* [Sxhkd](https://github.com/baskerville/sxhkd) \[Keybindings Manager\]
* [PrimeNote](https://gitlab.com/william.belanger/primenote) \[📝 Sticky notes!\]
* [AltTab](https://github.com/sagb/alttab) \[The window switcher for window managers!!\]
* [XFCE4 Power Manager](https://docs.xfce.org/xfce/xfce4-power-manager/) [As the Power Management Daemon]
* [Parcellite](https://github.com/rickyrockrat/parcellite) [Clipboard Manager]
* [Greenclip](https://github.com/erebe/greenclip) [Clipboard frontend using rofi.]
* [Light & Brightnessctl](https://haikarainen.github.io/light/) [Brightness controllers, Some scripts require these programs.]
* [Pamixer](https://github.com/cdemoulins/pamixer) [Pulseaudio command line mixer, again some scripts require this program.]
* [PipeWire](https://pipewire.org/) [I am using pipewire as my audio backend mainly due to the excellent bluetooth support it provides. **However, I think the scripts should work with pulseaudio** tho I have not tested it. Open an issue if you think they don't.]
* And various other programs you will need for the scripts to work. Look at the commands below:

#### 1. Installing Worm:
Installing WormWM is a bit weird, since it is written in nim, you will need to compile it using the Nim compiler, however using the nim that comes with your package manager on Fedora gave me an error [On ArchLinux it works fine, I have not tested it on ubuntu], so it is recommended to always grab the nim compiler binary directly. The steps are given below:

**1. On ArchLinux**
Simply run:
```bash
paru -S worm-git
```
to install it using your favorite AUR helper.

**2. On Fedora/Ubuntu**
We will be using choosenim for this purpose. To install it on your home folder, run this command: 
```bash
curl https://nim-lang.org/choosenim/init.sh -sSf | sh
```
Make sure you have curl installed, if not do `sudo apt install curl` for ubuntu, `sudo dnf install curl` for fedora and `sudo pacman -S curl` on ArchLinux.
It usually installs nim on `/home/your-username/.nimble/bin`. Next you will need to add it to your path, for that just copy and paste the command it gives you after the installation in your `.bashrc`, or if using fish, paste the command in `~/.config/fish/config.fish`. Restart your terminal afterwards.

After installing choosenim, we can finally move on to installing worm.

CD into your `~/Downloads` folder and run:
```shell
git clone https://github.com/codic12/worm.git && cd worm
```
Now run 
```shell
nimble build -d:release
```
to compile the window manager.

The static worm binary will be available to you after this step, just move it somewhere in your path,
For example,
```bash
sudo mv worm /usr/bin/worm
```

Since we are using SLiM Display Manager, we don't need an Xsession file for the window manager. Instead, SLiM relies on `~/.xinitrc` file, so we just need to append the `worm` command inside the file.

#### 2. Installing Elkowar's Wacky Widgets (EWW)
Next we need to install EWW widgets, but for that, we need the `nightly` version of the Rust programming language to be available on your system.

**1. On ArchLinux**
The rust nightly binary is available in the AUR, you can get it by
```bash
paru -S rust-nightly-bin
```

**2. On Fedora/Ubuntu**
For this purpose, we will be using `rustup`, a rust toolchain installer. Similar to choosenim, it has an installation script.
```bash
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
```
Now we need to customize the installation, select `2` to change how the installer installs rust. Press `Enter/Return` for `Default host triple`, next in order to switch to the nightly toolchain, type `nightly` when it asks for it. Just press `Enter/Return` for everything else and begin the installation. Rustup usually installs the compiler at `/home/your-username/.rustup` . Rustup automatically adds the compiler and the cargo packager to your path, in `.zshrc`, `.bashrc` or `.profile`.

Since we are done with installing Rust, it is time to install Eww!
CD into your `~/Downloads` folder and run:
```bash
git clone https://github.com/elkowar/eww.git && cd eww
```
Now, we will have to compile the Eww binary, it is always faster to allow rust to use all your threads when compiling, for that, run the following:
```bash
cargo build --release -j $(nproc)
```
The above command will build EWW using all the threads you have, this still usually takes 5-10 minutes on an 8 threads CPU. Once it is done compiling, we will need to move the EWW binary to your `/usr/bin` folder, just run
```bash
sudo mv target/release/eww /usr/bin/eww && sudo chmod +x /usr/bin/eww
```
This installs EWW to your `/usr/bin` folder and makes it executable.
With this we are done with the installation of EWW. Now we have to install the packages that we would need for the functioning of the config files.

**Installation of packages on ArchLinux:**
A one time command to install all these things with **your favorite AUR Helper** is given below, in this case we are using `paru`, you can any other, I don't mind 🙃

```bash
paru -S xdg-user-dirs git slim dunst tint2 jgmenu rofi alacritty nitrogen sxhkd i3lock-color picom-jonaburg-fix lua starship fish cava gtk brightnessctl zsh playerctl pipewire pipewire-pulse thunar
papirus-icon-theme curl jq libappindicator-gtk3 org.freedesktop.secrets light wireless_tools maim gpick mate-polkit yaru-sound-theme xdg-desktop-portal-gtk xdg-desktop-portal mpv jgmenu lxappearance xfce-power-manager xfce4-settings xprintidle vdirsyncer khal moreutils fortune-mod wmctrl bc unzip bluez bluez-utils imagemagick parcellite xclip pamixer pavucontrol rofimoji rofi-greenclip 
```

**Installation of packages on Fedora(36):**
```bash
sudo dnf install xdg-user-dirs git slim dunst tint2 jgmenu rofi alacritty nitrogen sxhkd lua starship fish cava gtk3 brightnessctl zsh playerctl pipewire pipewire-pulseaudio Thunar papirus-icon-theme curl jq libappindicator-gtk3 gnome-keyring light wireless-tools maim gpick mate-polkit yaru-sound-theme xdg-desktop-portal-gtk xdg-desktop-portal jgmenu lxappearance xfce-power-manager xfce4-settings vdirsyncer khal moreutils fortune-mod wmctrl bc unzip bluez ImageMagick parcellite xclip pavucontrol rofimoji
```

**NOTE:** Some packages are not available in the official fedora repositories, they can be installed using the methods below:
1. i3lock-color:
Refer to the official [README.md](https://github.com/Raymo111/i3lock-color) on how to install i3lock-color on your system.
2. Picom (Jonaburg Fix):
```bash
sudo dnf install dbus-devel gcc git libconfig-devel libdrm-devel libev-devel libX11-devel libX11-xcb libXext-devel libxcb-devel mesa-libGL-devel meson pcre-devel pixman-devel uthash-devel xcb-util-image-devel xcb-util-renderutil-devel xorg-x11-proto-devel
```
then do `git clone https://github.com/Arian8j2/picom-jonaburg-fix.git && cd picom-jonaburg-fix`.
then run:
```bash
git submodule update --init --recursive
meson --buildtype=release . build
ninja -C build
ninja -C build install 
```
to install Picom (Jonaburg Fix) to your system.
3. MPV:
For this you will need to enable RPM Fusion repository. 
Just do `sudo dnf install https://mirrors.rpmfusion.org/free/fedora/rpmfusion-free-release-36.noarch.rpm` to install RPM Fusion Free.
Then do `sudo dnf install mpv` to install MPV.
4. `xprintidle`
For this you need the RPM Sphere repository. 
Just do `sudo dnf install https://github.com/rpmsphere/noarch/raw/master/r/rpmsphere-release-36-1.noarch.rpm` to install the RPM Sphere Repository.
Then do `sudo dnf install xprintidle` to install xprintidle on your system.
5. Pulseaudio Mixer (pamixer):
Follow the installation instructions in the README of the project [here](https://github.com/cdemoulins/pamixer#installation)
6. Rofi Greenclip (Clipboard GUI):
Simply run `wget -O /usr/bin/greenclip https://github.com/erebe/greenclip/releases/download/v4.2/greenclip` to install Rofi Greenclip to your System.

**Installation of packages on Ubuntu(22.04 LTS):**
```bash
sudo apt install xdg-user-dirs git slim dunst tint2 jgmenu rofi nitrogen sxhkd lua fish cava brightnessctl zsh playerctl pipewire pipewire-pulse thunar papirus-icon-theme curl jq libayatana-appindicator3-1 gnome-keyring light wireless-tools maim gpick mate-polkit yaru-theme-sound xdg-desktop-portal-gtk xdg-desktop-portal mpv jgmenu lxappearance xfce-power-manager xfce4-settings xprintidle vdirsyncer khal moreutils fortune-mod wmctrl bc unzip bluez imagemagick parcellite xclip pavucontrol
```

**NOTE:** Some packages are not available in the official ubuntu repositories, they can be installed using methods below:
1. Alacritty:
```bash
sudo add-apt-repository ppa:aslatter/ppa
sudo apt update
sudo apt install alacritty
```
2. i3lock-color:
Refer to the official [README.md](https://github.com/Raymo111/i3lock-color) on how to install i3lock-color on your system.
3. Picom (Jonaburg Fix):
```bash
sudo apt install libxext-dev libxcb1-dev libxcb-damage0-dev libxcb-xfixes0-dev libxcb-shape0-dev libxcb-render-util0-dev libxcb-render0-dev libxcb-randr0-dev libxcb-composite0-dev libxcb-image0-dev libxcb-present-dev libxcb-xinerama0-dev libxcb-glx0-dev libpixman-1-dev libdbus-1-dev libconfig-dev libgl1-mesa-dev libpcre2-dev libpcre3-dev libevdev-dev uthash-dev libev-dev libx11-xcb-dev meson
```
then do `git clone https://github.com/Arian8j2/picom-jonaburg-fix.git && cd picom-jonaburg-fix`.
then run:
```bash
git submodule update --init --recursive
meson --buildtype=release . build
ninja -C build
ninja -C build install 
```
to install Picom (Jonaburg Fix) to your system.
4. Starship Prompt:
```bash
curl -fsSL https://starship.rs/install.sh | bash
```
Run this in your terminal to install starship prompt.
5. Pulseaudio Mixer (pamixer):
Follow the installation instructions in the README of the project [here](https://github.com/cdemoulins/pamixer#installation)
6. RofiMoji (Emoji Selector)
Make sure you have `python3-pip` installed.
Then run `sudo pip3 install rofimoji` to install rofimoji.
7. Rofi Greenclip (Clipboard GUI):
Simply run `wget -O /usr/bin/greenclip https://github.com/erebe/greenclip/releases/download/v4.2/greenclip ` to install Rofi Greenclip to your System

### 2\.Theming And Appearance:
#### 1. Fonts:
The fonts can be found [fonts](https://github.com/syndrizzle/hotfiles/tree/worm/usr/share/fonts) directory. Just run `sudo mv * /usr/share/fonts` to install them to your system.
* **UI Font:** Proxima Nova
* **Monospace Font:** JetBrainsMono Nerd Font

Icon Based Fonts:

* **[Material Design Icons Enhanced:](https://github.com/Templarian/MaterialDesign-Font/blob/master/MaterialDesignIconsDesktop.ttf)** The icon font used by major stuff.
* **[Google's Material:](https://github.com/google/material-design-icons/tree/master/font)** Google's material font used by some stuff as well.
* **[Fluency/Favorites:](https://github.com/syndrizzle/hotfiles/tree/worm/usr/share/fonts/Fluency.ttf)** A custom font created by me using icons8's font creation system used by the bottom bar and some other stuff.
* **[System:](https://github.com/syndrizzle/hotfiles/tree/worm/usr/share/fonts/System.ttf)** Font with general system icons used by the bottom bar to display status of stuff.
* [Feather:](https://github.com/syndrizzle/hotfiles/tree/worm//usr/share/fonts/Feather.ttf)A really awesome icon font.

Icons: Icons used by some things can be found in [icons](https://github.com/syndrizzle/hotfiles/tree/worm/.local/icons) directory.

#### 2\. Icon Theme:
The Icon theme used in these config files is [Papirus icon theme](https://github.com/PapirusDevelopmentTeam/papirus-icon-theme) but with catppuccin flavored Folders.
After installing Papirus icon theme from the commands above in the installation section, it is time to install the folders!
To install them, simply run the following commands:
```bash
git clone https://github.com/catppuccin/papirus-folders.git
cd papirus-folders
sudo cp -r src/* /usr/share/icons/Papirus  
./papirus-folders -C cat-blue --theme Papirus-Dark
```
This modifies the papirus icon theme to use the catppuccin themed folders.

Next is the cursor icon theme. 
We are using the catppuccin cursors so this can blend well with the icon theme.
```bash
git clone https://github.com/catppuccin/cursors.git
cd cursors
cd cursors
tar -xvzf Catppuccin-Blue-Cursors.tar.gz
sudo mv Catppuccin-Blue-Cursors /usr/share/icons/
```
This installs the cursor theme in the root directory for all users.

#### 3. Theming:
*   **Colorscheme:** [Catppuccin](https://github.com/catppuccin/catppuccin/)
*   **GTK Theme:** [Catppuccin GTK Theme](https://github.com/catppuccin/gtk)
*   **Wallpapers:** Taken from Pexels modified using [catppuccin-factory](https://github.com/Fxzzi/catppuccin-factory) All the wallpapers can be found in the `.wallpapers` directory.
*   **SliM Theme:** [Catppuccin SLiM](https://github.com/syndrizzle/hotfiles/tree/worm/usr/share/slim/themes/catppuccin) (🖌️ Made with figma)
	**NOTE:** Since the SLiM theme is just a glued picture of images and text on it, the theme is **not dynamic**. So you will need to edit the SLiM theme yourself using Figma (It is very easy trust me). You can find the figma file [here](https://www.figma.com/file/i2PIFn8hfkX71CsnMtViWv/SLiM-login-template)
	If you need any help, feel free to contact me on discord (Just a reminder, my discord username is `Syndrizzle#3826`) and I will happily provide you the needed help :)

### 3. Installing the configuration files:
Welcome to the lovely part, the part you've been waiting for! It is time to install the configuration files.

Clone the repository and CD into it :
```bash
git clone https://github.com/syndrizzle/hotfiles && cd hotfiles
```
Next, as easy as looking at the pictures, just move the folders to their respective places:
```bash
cp -r .config .cache .fonts .local .icons .scripts .wallpapers ~/
sudo cp -r usr/ /
sudo cp etc/slim.conf /etc/slim.conf
cp .gtkrc-2.0 .vimrc .xinitrc .Xresources ~/
```
This installs all the configuration files to their respective places. Reboot your machine and see the magic 🚀✨

# Gallery
=======
![](https://i.imgur.com/a8JVUKH.png)

##### Main Desktop 💻
Conky, jgmenu and Eww widgets

![](https://i.imgur.com/OAVJTu8.png)

##### A Control Center 🚀
A beautiful notifiation center you can see while having your morning coffe ☕

![](https://i.imgur.com/YxEhIP4.png)

##### A Dashboard
Gives you all the info you'll ever need!

![](https://i.imgur.com/QKhjVwY.png)

##### A Sexy Greeter 💯
Tho stuff here except the user and password is glued lol. You can find the figma files for changing it at the top of this readme though.

![](https://i.imgur.com/4EdbJL9.png)

##### A Lock screen 🔒
This is taken from [u/dark\_dryu's configs](https://www.reddit.com/r/unixporn/comments/pikl7s/i3lockeww_lock_screen_with_widgets/) found on this reddit post.

![](https://i.imgur.com/0PftxXh.png)

#### The Powermenu
Yes, it is usable, like..obviously.

Tips and Tricks
=======

### 1. Get your Reddit Karma
We are using the Python Reddit Api Wrapper (PRAW) for this. You will need a reddit account with the Client ID and the Client Secret of your Reddit App.
To create a Reddit application, you will need to register it on https://www.reddit.com/prefs/apps/ by clicking `Create another app`. Give it a name and set redirect and about URLs to `https://localhost:8080`. Then copy it's client secret and it's client ID.
Install the PRAW library by executing `pip3 install praw`. Then edit the configuration file located as `~/.config/eww/Main/scripts/karma`. You will also need to fill in your reddit username and the password.

### 2. Get your E-Mails
If you use GMail, you can get your E-Mails to be shown on the dashboard as well!
For this we are using the `imaplib` python module.
You will need to enable Two Factor Authentication on your Google Account. Then head over to https://accounts.google.com/security, scroll down and select App Passwords. Under the `Select app` dropdown menu select `Mail`, and give it a name. Then copy that password. This is the password we will be using to authenticate IMAP with G-Mail, we are not done yet.
Head over to https://mail.google.com/mail/#settings/fwdandpop and click on the `enable IMAP` button, then scroll down and click on `Save Changes`. 
Edit the Mail file located as `~/.config/eww/Main/scripts/mail` and fill in your G-Mail ID and the App password we created earlier.

### 3. Rofi for everything!
You can use rofi for selecting emojis, view your clipboard and even view the keybindings incase you forget something!
To bring up the emoji selector, press <kbd>Ctrl</kbd> + <kbd>.</kbd>
To bring up the Keybindings viewer, press <kbd>Ctrl</kbd> + <kbd>/</kbd>
**NOTE:** You will need to move the `sxhkd-keys` script to your `/usr/bin/` folder:
```bash
sudo mv ~/.scripts/sxhkd-keys /usr/bin/sxhkd-keys
```
To bring up the clipboard viewer, press  <kbd>Windows/CMD Key</kbd> + <kbd>V</kbd>

### 4. Get your thunar sidepane folders back!
Usually you end up with no folders on the thunar's sidepane when using a window managers, for this you just need to run:
```bash
xdg-user-dirs-update
```
in your terminal and reboot, this regenerates your XDG folders. now right click on the folder you want to move and select `Send to > Side Pane`, or you can just drag and drop them.

### Lockscreen and Greeter Stuff
In order to make XFCE4 Power Manager recognize that you have a lock screen, just run this command:
```bash
xfconf-query -c xfce4-session -p /general/LockCommand -s "$HOME/.scripts/lock-run" --create -t string
```
What this does is it runs this command whenever you call the lock screen, like by closing the lid, suspend...etc.

Make sure to change the default username in `/etc/slim.conf` from `syndrizzle` to your username.

## Stargazers over time [![Stargazers over time](https://starchart.cc/Syndrizzle/hotfiles.svg)](https://starchart.cc/Syndrizzle/hotfiles)
Credits
=======

Thanks to [pagankeymaster](https://github.com/pagankeymaster) for helping me to achieve this rice. That notification center couldn't have been made possible without this man's work.

*   Thanks again to all the members of the [r/unixporn discord server](https://discord.gg/d53yESY) who helped me.
*   This rice was inspired by many other rices out there, like [the-glorious-dotfiles](https://github.com/manilarome/the-glorious-dotfiles/), [Axarva's xmonad rice](https://github.com/Axarva/dotfiles-2.0), [adi1090x's widgets](https://github.com/adi1090x/widgets/). Thanks to all of them for their work!
*   All the icons are from [icons8](https://icons8.com), [flaticon](https://flaticon.com), [iconscout](https://iconscout.com) and various other sources (Sorry I don't remember each one of them :/)

#### Stuff I have taken from repositories:

*   The lock screen is just a modified version of a lock screen I spotted on reddit, [here is the post](https://www.reddit.com/r/unixporn/comments/pikl7s/i3lockeww_lock_screen_with_widgets/) and [here is the repo.](https://github.com/dark-dryu/i3lock-widgets-config/)
* The Quotes for the power menu were taken from [the-glorious-dotfiles](https://github.com/manilarome/the-glorious-dotfiles) and the default image was taken from [this dribble post](https://dribbble.com/shots/17124030-We-Gonna-Make-Dribbble-Shots-WGMInterfaces-Derivative-art)
*   Scripts were taken from:
    1.  [vile](https://github.com/pagankeymaster/vile/tree/rewrite) by [pagankeymaster](https://github.com/pagankeymaster/) that contained needed stuff for the control center's notification part.
    2.  [adi1090x's widgets](https://github.com/adi1090x/widgets/) for eww.
    3.  [t2ec](https://github.com/nwg-piotr/t2ec) and [psuinfo](https://github.com/nwg-piotr/psuinfo) by nwg-piotr that helped me to frame the tray for status items on the bottom bar.
    4.  [Axarva's xmonad rice](https://github.com/Axarva/dotfiles-2.0) again for eww.
    5.  The conky was taken from the [Leonis](https://github.com/closebox73/Leonis) theme pack by [closebox73](https://github.com/closebox73/), modified for the use with everblush.
    6.  And various other repositories, if I have left one or more, please open and issue so I can include them.