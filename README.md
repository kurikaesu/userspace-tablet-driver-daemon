# Kuri's Userspace tablet driver daemon

This repo currently houses a command-line only utility that creates a userspace driver for the following models:
- Artist 22E Pro
- Artist 22R Pro
- Artist 13.3 Pro
- Artist 12 Pro (Unconfirmed, needs testing)
- Artist 24 Pro (Unconfirmed, needs testing)
- Deco Pro S (Use GUI to switch touch-pad modes)
- Deco Pro M (Use GUI to switch touch-pad modes)
- Deco 01v2
- Huion WH1409 v2
- Huion WH1409 (2048 pressure level version)
- Huion H1161

It initializes the supported tablets with their default bindings but the bindings can be changed by modifying the configuration file:
`$HOME/.local/share/userspace_tablet_driver_daemon/driver.cfg`

This driver also listens to a unix socket at `$HOME/.local/var/run/userspace_tablet_driver_daemon.sock` that takes in messages of format `struct unix_socket_message`: https://github.com/kurikaesu/userspace-tablet-driver-daemon/blob/main/src/unix_socket_message.h. It is possible to receive a response from messages sent to devices as long as the response expected flag is set.

Things on the TODO list:
- Support more devices

Things on the long-term TODO list:
- Create an X11 driver so that we don't have to piggy-back on wacom

## Warning
- This includes a 70-uinput-plugdev.rules file that gives users on your computer that are in the `plugdev` permission group access to uinput without SUDO. This is how I can make this driver run without having the user constantly enter their password each time.

## How to change bindings
Preferred way is to use the GUI: https://github.com/kurikaesu/userspace-tablet-driver-gui
You can change bindings manually by changing the JSON config but the format is currently changing too quickly to make effective documentation.

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
```

You can optionally `sudo make install` which will place the executable in /usr/bin/userspace_tablet_driver_daemon.
If you opted not to do the `make install` then the configuration files under `config` will need to be placed in their respective locations. When this is done, just reboot your computer and all the rules required to make things work should be in place.

I would suggest running the command userspace_tablet_driver_daemon from the terminal first and watching the output to see whether or not things are broken first before having your desktop environment auto-start the application on login.

## Note
This driver leverages the `wacom` x11 drivers to handle the stylus/digitizer. You will need to use `xsetwacom` to configure the digitizer side of things.

## Contributing
Should you want to contribute there are a few ways to do so.
- Testing the driver on your Linux distribution. If it doesn't work, cut me an issue.
- Fix an open issue and send me a pull-request (If needed)
- Implement support for your tablet if you are able to. Otherwise,
- Lend me your tablet so that I can do the implementation or
- Buy the tablet for me from my wishlist: https://www.amazon.com/hz/wishlist/ls/10J2MJJPCC2JF?ref_=wl_share
- If you think a tablet should be on my wishlist, but it isn't, let me know!
