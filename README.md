# Kuri's XP-Pen Userland drivers

This repo currently houses a command-line only utility that creates a userspace driver for the following models:
- Artist 22R Pro
- Artist 13.3 Pro
- Artist 12 Pro (Unconfirmed, needs testing)
- Artist 24 Pro (Unconfirmed, needs testing)
- Deco Pro S (You will need to use the proprietary driver to change the touch type to "Customized" in order to get bindings)
- Deco Pro M (Same deal as small version)

It initializes the supported tablets with their default bindings but the bindings can be changed by modifying the configuration file:
`$HOME/.local/share/xp_pen_userland/driver.cfg`

This driver also listens to a unix socket at `$HOME/.local/var/run/xp_pen_userland.sock` that takes in messages of format `struct unix_socket_message`: https://github.com/kurikaesu/xp-pen-userland/blob/main/src/unix_socket_message.h. It is possible to receive a response from messages sent to devices as long as the response expected flag is set.

Things on the TODO list:
- Make a nice GUI to perform binding changes. Current GUI is not "nice" but it is here: https://github.com/kurikaesu/xp-pen-userland-config-util
- Support more XP-Pen devices

## Warning
- This includes a 70-uinput-plugdev.rules file that gives users on your computer that are in the `plugdev` permission group access to uinput without SUDO. This is how I can make this driver run without having the user constantly enter their password each time.

## How to change bindings
Preferred way is to use the GUI: https://github.com/kurikaesu/xp-pen-userland-config-util
You can change bindings manually by changing the JSON config but the format is currently changing too quickly to make effective documentation.

## Building
This uses cmake to generate the required makefiles so make sure to have that installed.
On debian/ubuntu systems it can be done by:
`sudo apt install cmake`

With cmake installed it is a matter of:
```
git clone https://github.com/kurikaesu/xp-pen-userland.git
cd xp-pen-userland
cmake .
make
```

You can optionally `sudo make install` which will place the executable in /usr/bin/xp_pen_userland.
If you opted not to do the `make install` then the configuration files under `config` will need to be placed in their respective locations. When this is done, just reboot your computer and all the rules required to make things work should be in place.

I would suggest running the command xp_pen_userland from the terminal first and watching the output to see whether or not things are broken first before having your desktop environment auto-start the application on login.

## Note
This driver leverages the `wacom` x11 drivers to handle the stylus/digitizer. You will need to use `xsetwacom` to configure the digitizer side of things.