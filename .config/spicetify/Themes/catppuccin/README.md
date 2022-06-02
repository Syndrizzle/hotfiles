<h3 align="center">
	<img src="https://raw.githubusercontent.com/catppuccin/catppuccin/dev/assets/logos/exports/1544x1544_circle.png" width="100" alt="Logo"/><br/>
	<img src="https://raw.githubusercontent.com/catppuccin/catppuccin/dev/assets/misc/transparent.png" height="30" width="0px"/>
	Catppuccin for Spicetify
	<img src="https://raw.githubusercontent.com/catppuccin/catppuccin/dev/assets/misc/transparent.png" height="30" width="0px"/>
</h3>

<p align="center">
    <a href="https://github.com/davidbgonz/spicetify/stargazers"><img src="https://img.shields.io/github/stars/davidbgonz/spicetify?colorA=1e1e28&colorB=c9cbff&style=for-the-badge&logo=starship style=for-the-badge"></a>
    <a href="https://github.com/davidbgonz/spicetify/issues"><img src="https://img.shields.io/github/issues/davidbgonz/spicetify?colorA=1e1e28&colorB=f7be95&style=for-the-badge"></a>
    <a href="https://github.com/davidbgonz/spicetify/contributors"><img src="https://img.shields.io/github/contributors/davidbgonz/spicetify?colorA=1e1e28&colorB=b1e1a6&style=for-the-badge"></a>
</p>

![Spicetify Theme Preview](assets/preview.png)

## Usage

1. Follow the installation instructions for [Spicetify](https://spicetify.app/docs/getting-started)
   * Note: The installation instructions for linux have you setting `/opt/spotify` and `/opt/spotify/Apps/*` permissions to `777`. This is not good practice and should be avoided. If you want, you can set the group ownership for these paths to one that your user is apart of (i.e. `users`), or you can create a new group and add it as a secondary group to your user. After that you can use the following command to give access to the group:
      ```
      GROUP=<group_name>
      sudo chgrp $GROUP /opt/spotify
      sudo chgrp -R $GROUP /opt/spotify/Apps
      sudo chmod 775 /opt/spotify
      sudo chmod 775 -R /opt/spotify/Apps
      ```
2. Follow the [basic usage](https://spicetify.app/docs/getting-started#basic-usage) to setup Spicetify
3. Either clone this repo `git clone https://github.com/catppuccin/spicetify.git ~/.config/spicetify/Themes/catppuccin` or copy over the `color.ini` and `user.css` files manually to a directory named `catppuccin` in the Spicetify theme directory. This is usually located in `~/.config/spicetify/Themes`
4. Link over the `catppuccin.js` extension to the appropriate place.
   ```
   SPICE_PATH="$HOME/.config/spicetify"
   ln -s $SPICE_PATH/Themes/catppuccin/catppuccin.js $SPICE_PATH/Extensions/
   ```
4. Set theme and color scheme. Supported color schemes: `rosewater`, `flamingo`, `mauve`, `pink`, `maroon`, `red`, `peach`, `yellow`, `green`, `teal`, `blue`, `sky`, `lavender`
   ```
   spicetify config current_theme catppuccin
   spicetify config color_scheme lavender
   spicetify config inject_css 1 replace_colors 1 overwrite_assets 1
   spicetify config extensions catppuccin.js
   ```
5. If you want to update the theme without opening it up run `spicetify update`. If you want to update the theme and open/restart Spotify at the same time run `spicetify apply`

## 📜 License

Catppuccin is released under the MIT license, which grants the following permissions:

-   Commercial use
-   Distribution
-   Modification
-   Private use

For more convoluted language, see the [LICENSE](https://github.com/catppuccin/catppuccin/blob/main/LICENSE).

## 💝 Thanks to

- [davidbgonz](https://github.com/davidbgonz)
- [OlaoluwaM](https://github.com/OlaoluwaM)

<p align="center"><img src="https://raw.githubusercontent.com/catppuccin/catppuccin/dev/assets/footers/gray0_ctp_on_line.svg?sanitize=true" /></p>
<p align="center">Copyright &copy; 2022-present <a href="https://github.com/catppuccin" target="_blank">Catppuccin Org</a>
<p align="center"><a href="https://github.com/catppuccin/catppuccin/blob/main/LICENSE"><img src="https://img.shields.io/static/v1.svg?style=for-the-badge&label=License&message=MIT&logoColor=d9e0ee&colorA=302d41&colorB=c9cbff"/></a></p>
