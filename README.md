<p align="center">
  <img     width="100"
    height="100"
    src="https://i.imgur.com/ZKxBsFu.png"
  >
</p>
<h6 align="center">The Great Keanu<h6>

## <h1 align="center">Hotfiles 🔥</h1>

### Thanks for visiting!
<p align="center">
  <img src="https://i.imgur.com/ZKxBsFu.png" height="100" width="100">
</p>
<h6 align="center">The Great Keanu</h6><h6>

</h6><h2 class="atx" id="lessh1-aligndouble-quotecenterdouble-quotegreaterhotfiles-🔥lessh1greater"></h2><h1 align="center">Hotfiles 🔥</h1>
<h3 class="atx" id="thanks-for-visiting">Thanks for visiting!</h3>
<p>Welcome to my GitHub repository of personal dotfiles! 😃
Here you can find configuration files for various desktops, tho this is still a Work-In-Progress thing.</p>
<p align="center"><a href="#setup"><img alt="" title="" src="https://i.imgur.com/F4IPhSj.png"></a>     <a href="#credits"><img alt="" title="" src="https://i.imgur.com/xN8xUan.png"></a>      <a href="#gallery"><img alt="" title="" src="https://i.imgur.com/SmAepSb.png"></a></p>

<h4 class="atx" id="1-🪱-the-wabbling-worm-🪱">1. 🪱 The Wabbling Worm 🪱</h4>
<h3 class="atx" id="about">About</h3>
<ul>
<li><strong>Alacritty</strong> as the terminal.</li>
<li><strong>WormWM</strong> as the window manager.</li>
<li><strong>Picom</strong> as the compositor.</li>
<li><strong>EWW</strong> as the widgets [Dashboard, Bar and Sidebar]</li>
<li><strong>Rofi</strong> as the application launcher.</li>
<li><strong>SLiM</strong> as the Display Manager.</li>
<li><strong>Dunst</strong> as the notification daemon.</li>
<li><strong>Conky</strong> as the desktop eyecandy.</li>
<li><strong>jgmenu</strong> as the desktop root menu.</li>
</ul>
<p>⚠️ <strong>NOTE!!!!!</strong>
This configuration was made for my PC, so things here might not work on your PC, in that case, please try if you can fix that up, or you can open an issue for help :)
This was made for a <strong>1920x1080</strong> screen and on a <strong>Laptop</strong> with <strong>96</strong> dpi.</p>
<h3 class="atx" id="🔔-figma-files">🔔 Figma Files:</h3>
<h5 class="atx" id="there-are-some-figma-files-that-you-might-need-while-configuring-the-dotfiles">There are some figma files that you might need while configuring the dotfiles.</h5>
<ul>
<li>The lock screen (Since it is just glued, lol.) -&gt; <a href="https://www.figma.com/file/i2PIFn8hfkX71CsnMtViWv/SLiM-login-template">Figma</a></li>
<li>The Conky background -&gt; <a href="https://www.figma.com/file/T4pKJ1IyOj2M1csbP7WEIU/Conky-Alterf-Everblush">Figma</a></li>
<li>The cursors -&gt; <a href="https://www.figma.com/file/4t5745WuhQwWoXtGiTyhcT/Everblush-macOSBigSur">Figma</a></li>
</ul>
<h1 class="atx" id="setup">Setup</h1>
<h4 class="atx" id="1-packages">1. Packages:</h4>
<p>This is not a guide on installing these configuration files, instead it just lists stuff that has been used in these configs so you can install them. However if you need help, feel free to DM me on Discord: <code>Syndrizzle#3826</code> or you can open an issue on github. 😊</p>
<p>For this setup, you need the following packages:</p>
<ul>
<li><a href="https://github.com/codic12/worm">WormWM</a> [The main window manager :p]</li>
<li><a href="https://github.com/elkowar/eww">EWW</a> [😮 For the beautiful things!]</li>
<li><a href="https://github.com/brndnmtthws/conky">Conky</a> [For desktop widget]</li>
<li><a href="https://github.com/dunst-project/dunst">Dunst</a> [For Notifications]</li>
<li><a href="https://gitlab.com/o9000/tint2">tint2</a> [For System Tray]</li>
<li><a href="https://github.com/johanmalm/jgmenu">jgmenu</a> [For root desktop menu]
For jgmenu, pleae edit the <code>~/.config/jgmenu/prepend.csv</code> to change the username of the home directory from <code>syndrizzle</code> to your linux username, since jgmenu only recognizes absolute paths for some reason.</li>
<li><a href="https://github.com/davatorium/rofi">rofi</a> [For the application launcher]</li>
<li><a href="https://github.com/alacritty/alacritty">Alacritty</a> [🚀 Blazing fast terminal emulator]</li>
<li><a href="https://github.com/l3ib/nitrogen">Nitrogen</a> [Wallpaper app]</li>
<li><a href="https://github.com/Arian8j2/picom-jonaburg-fix">Picom (Jonaburg Fix)</a> [The compositor! But with fixes to support shadows with blur ✨]</li>
<li><a href="https://github.com/baskerville/sxhkd">Sxhkd</a> [Keybindings Manager]</li>
<li><a href="https://gitlab.com/william.belanger/primenote">PrimeNote</a> [📝 Sticky notes!]</li>
</ul>
<p>Additionally, some other stuff that I used to minimize my time in creating scripts for stuff:</p>
<ul>
<li><a href="https://github.com/nwg-piotr/t2ec">t2ec</a></li>
<li><a href="https://github.com/nwg-piotr/psuinfo">psuinfo</a></li>
</ul>
<h4 class="atx" id="2-fonts">2. Fonts:</h4>
<p>This is a list of all the fonts used in this setup. You can find all of them in the <a href="https://github.com/syndrizzle/hotfiles/tree/worm/.fonts">.fonts</a> directory or the <a href="https://github.com/syndrizzle/hotfiles/tree/worm/usr/share/fonts">root fonts</a> directory. Tho I am still listing them in case I've missed any.</p>
<ul>
<li><strong>UI Font:</strong> Proxima Nova</li>
<li><strong>Monospace Font:</strong> JetBrainsMono Nerd Font</li>
</ul>
<p>Icon Based Fonts:</p>
<ul>
<li><strong><a href="https://github.com/Templarian/MaterialDesign-Font/blob/master/MaterialDesignIconsDesktop.ttf">Material Design Icons Enhanced:</a></strong> The icon font used by major stuff.</li>
<li><strong><a href="https://github.com/google/material-design-icons/tree/master/font">Google's Material:</a></strong> Google's material font used by some stuff as well.</li>
<li><strong><a href="https://github.com/syndrizzle/hotfiles/tree/worm/.fonts/Fluency.ttf">Fluency/Favorites:</a></strong> A custom font created by me using icons8's font creation system used by the bottom bar and some other stuff.</li>
<li><strong><a href="https://github.com/syndrizzle/hotfiles/tree/worm/.fonts/System.ttf">System:</a></strong> Font with general system icons used by the bottom bar to display status of stuff.</li>
</ul>
<p>Icons:
Icons used by some things can be found in <a href="https://github.com/syndrizzle/hotfiles/tree/worm/.local/icons">icons</a> directory.</p>
<h4 class="atx" id="3-theming">3. Theming:</h4>
<ul>
<li><strong>Colorscheme:</strong> <a href="https://github.com/Everblush/">Everblush</a></li>
<li><strong>GTK Theme:</strong> <a href="https://github.com/Everblush/gtk">Everblush GTK</a></li>
<li><strong>Cursor Theme:</strong> <a href="https://github.com/syndrizzle/hotfiles/tree/worm/usr/share/icons/macOSBigSur-%5BEverblush%5D/">macOSBigSur-[Everblush]</a></li>
<li><strong>Wallpaper:</strong> <a href="https://github.com/syndrizzle/hotfiles/tree/worm/.wallpapers/Bridge.png">Bridge</a> (Wallpaper taken from pexels sprinkled with everblush colors)
<strong>Note:</strong> you can find more wallpapers in the <code>.wallpapers</code> folder.</li>
<li><strong>SliM Theme:</strong> <a href="https://github.com/syndrizzle/hotfiles/tree/worm/usr/share/slim/themes/everblush">Everblush</a> (🖌️ Made with figma)</li>
</ul>
<h5 class="atx" id="nice-desktop-gib-screenshots-plz-p">Nice Desktop! Gib screenshots plz :p</h5>
<h1 class="atx" id="gallery">Gallery</h1>
<p align="center"><img src="https://i.imgur.com/OWyEWHE.jpg" alt="picture 1"></p>
<h5 align="center">Main Desktop 🖥️</h5>
<p align="center">Conky, jgmenu and Eww widgets</p>

<p align="center"><img src="https://i.imgur.com/nYFg8bD.png" alt="picture 2"></p>
<h5 align="center">A Control Center 🚀</h5>
<p align="center">A beautiful notifiation center you can see while having your morning coffe ☕</p>

<p align="center"><img src="https://i.imgur.com/c7cNQMc.png" alt="picture 3"></p>
<h5 align="center">A Dashboard ℹ️</h5>
<p align="center">Gives you all the info you'll ever need!</p>

<p align="center"><img src="https://i.imgur.com/Jw6VFbg.png" alt="picture 4"></p>
<h5 align="center">A Sexy Greeter 💯</h5>
<p align="center">Tho stuff here except the user and password is glued lol. You can find the figma files for changing it at the top of this readme though.</p>

<p align="center"><img src="https://i.imgur.com/gjXw0i3.jpg" alt="picture 5"></p>
<h5 align="center">A Lock screen 🔒</h5>
<p align="center">Turns out you can use it when your PC is idle 😲. This is taken from <a href="https://www.reddit.com/r/unixporn/comments/pikl7s/i3lockeww_lock_screen_with_widgets/">u/dark_dryu's configs</a> found on this reddit post.</p>  
<br>

<p>It's time for...Credits</p>
<h1 class="atx" id="credits">Credits</h1>
<p>Thanks to <a href="https://github.com/pagankeymaster">pagankeymaster</a> for helping me to achieve this rice. That notification center couldn't have been made possible without this man's work.</p>
<ul>
<li>Thanks again to all the members of the <a href="https://discord.gg/d53yESY">r/unixporn discord server</a> who helped me.</li>
<li>This rice was inspired by many other rices out there, like <a href="https://github.com/manilarome/the-glorious-dotfiles/">the-glorious-dotfiles</a>, <a href="https://github.com/Axarva/dotfiles-2.0">Axarva's xmonad rice</a>, <a href="https://github.com/adi1090x/widgets/">adi1090x's widgets</a>. Thanks to all of them for their work!</li>
<li>All the icons are from <a href="https://icons8.com">icons8</a>, <a href="https://flaticon.com">flaticon</a>, <a href="https://iconscout.com">iconscout</a> and various other sources (Sorry I don't remember each one of them :()</li>
</ul>
<h4 class="atx" id="stuff-i-have-taken-from-repositories">Stuff I have taken from repositories:</h4>
<ul>
<li>The lock screen is just a modified version of a lock screen I spotted on reddit, <a href="https://www.reddit.com/r/unixporn/comments/pikl7s/i3lockeww_lock_screen_with_widgets/">here is the post</a> and <a href="https://github.com/dark-dryu/i3lock-widgets-config/">here is the repo.</a></li>
<li>Scripts were taken from:<ol>
<li><a href="https://github.com/pagankeymaster/vile/tree/rewrite">vile</a> by <a href="https://github.com/pagankeymaster/">pagankeymaster</a> that contained needed stuff for the control center's notification part.</li>
<li><a href="https://github.com/adi1090x/widgets/">adi1090x's widgets</a> for eww.</li>
<li><a href="https://github.com/nwg-piotr/t2ec">t2ec</a> and <a href="https://github.com/nwg-piotr/psuinfo">psuinfo</a> by nwg-piotr that helped me to frame the tray for status items on the bottom bar.</li>
<li><a href="https://github.com/Axarva/dotfiles-2.0">Axarva's xmonad rice</a> again for eww.</li>
<li>The conky was taken from the <a href="https://github.com/closebox73/Leonis">Leonis</a> theme pack by <a href="https://github.com/closebox73/">closebox73</a>, modified for the use with everblush.</li>
<li>And various other repositories, if I have left one or more, please open and issue so I can include them.</li>
</ol>
</li>
</ul>

