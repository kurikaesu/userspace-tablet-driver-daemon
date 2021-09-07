# Kuri's XP-Pen Userland drivers

This repo currently houses a command-line only utility that creates a userspace driver for the following models:
- Artist 22R Pro

It initializes the supported tablets with their default bindings but the bindings can be changed by modifying the configuration file:
`$HOME/.local/share/xp_pen_userland/driver.cfg`

Things on the TODO list:
- Add binding support for 22R Pro dials
- Make a nice GUI to perform binding changes. Current GUI is not "nice" but it is here: https://github.com/kurikaesu/xp-pen-userland-config-util
- Support more XP-Pen devices

## Warning
- This includes a 70-uinput-plugdev.rules file that gives users on your computer that are in the `plugdev` permission group access to uinput without SUDO. This is how I can make this driver run without having the user constantly enter their password each time.

## How to change bindings
Preferred way is to use the GUI: https://github.com/kurikaesu/xp-pen-userland-config-util
You can change bindings manually by changing the JSON config but the format is currently changing too quickly to make effective documentation.
