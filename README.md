# Kuri's Linux user-space graphics tablet driver daemon

This repo currently houses a command-line only utility that creates a user-space driver for the following supported models:
- XP-Pen Artist 24 Pro
- XP-Pen Artist 22E Pro
- XP-Pen Artist 22R Pro
- XP-Pen Artist 16 Pro
- XP-Pen Artist 13.3 Pro
- XP-Pen Artist 12 Pro
- XP-Pen Artist 12 (2nd Gen)
- XP-Pen Innovator 16
- XP-Pen Deco Pro S (Use GUI to switch touch-pad modes)
- XP-Pen Deco Pro M (Use GUI to switch touch-pad modes)
- XP-Pen Deco 01v2
- XP-Pen Deco 03
- XP-Pen Deco mini7/mini7w
- XP-Pen Star G430S
- XP-Pen AC19 Shortcut Remote
- Huion Kamvas Pro 13
- Huion WH1409 v2
- Huion WH1409 (2048 pressure level version)
- Huion H1161
- Huion KD100 mini Keydial
- Gaomon M10K Pro
- Gaomon M10K 2018

### There is an untested generic XP-Pen tablet driver that should work for the majority of devices that aren't officially supported.

If you test your device against this generic driver and find that it works, please create an issue and provide the device ID for your device, its name and preferrably a link to the product page.
If your device only partially works, we will be interested in knowing that too.

---

## How to change bindings
Preferred way is to use the GUI: https://github.com/kurikaesu/userspace-tablet-driver-gui
You can change bindings manually by changing the JSON config but the format is currently changing too quickly to make effective documentation.

## To-do:
- Officially support more devices
- Provide a way to set up calibrate pressure curves through the GUI in a user friendly way.

## Warning
- This includes a 70-uinput-plugdev.rules file that gives users on your computer that are in the `plugdev` permission group access to uinput without SUDO. This is how I can make this driver run without having the user constantly enter their password each time.

## Installing
For debian based distributions, download the deb package from the list of releases. The deb package can be installed with `sudo dpkg -i <package_name.deb>`

If you use an Arch-based distro, you can install the package from the AUR. There are two versions, one unstable and one stable.

The unstable version can be installed with:

```bash
yay -S userspace-tablet-driver-daemon-git
```

The stable (release) version can be installed with:

```bash
yay -S userspace-tablet-driver-daemon
```

## Building
This uses cmake to generate the required makefiles so make sure to have that installed.
On debian/ubuntu systems it can be done by:
`sudo apt install cmake`

With cmake installed it is a matter of:
```
git clone https://github.com/kurikaesu/userspace-tablet-driver-daemon.git
cd userspace-tablet-driver-daemon
cmake .
make
sudo make install
```

The first time you `sudo make install` you will need to trigger the udev rule changes in order for the driver to work.
This can be done with:
```
sudo udevadm trigger
```

I would suggest running the command userspace_tablet_driver_daemon from the terminal first and watching the output to see if things are broken before having your desktop environment auto-start the application on login.

## Changing which display the device is mapped to
Use xinput in order to configure this:
```
xinput map-to-output <xinput device name> <xrandr-monitor-name>
```

You can get the monitor name by:
```
xrandr --listmonitors
```
It will show something like `eDP-1` or `HDMI-A-0`

#### Gnome under wayland without X11
Unfortunately at the moment I do not know how to change the display mapping when using Gnome under wayland as I can't find any documentation on how to instruct the compositor `mutter` to do so.

#### KDE under wayland without X11
This is the same situation as the Gnome desktop environment. There is work to add a new KCM to support libinput graphics tablets but it is not complete. See: https://phabricator.kde.org/T14971

## Contributing
Should you want to contribute there are a few ways to do so:
- Testing the driver on your Linux distribution. If it doesn't work, cut me an issue.
- Fix an open issue and send me a pull-request (If needed)
- Implement support for your tablet if you are able to. Otherwise,
- Lend me your tablet so that I can do the implementation or
- Buy the tablet for me from my wishlist: https://www.amazon.com/hz/wishlist/ls/10J2MJJPCC2JF?ref_=wl_share
- If you think a tablet should be on my wishlist, but it isn't, let me know!
- Add suggestions to the issues. We can discuss their feasibility and at what milestones we should aim for.
