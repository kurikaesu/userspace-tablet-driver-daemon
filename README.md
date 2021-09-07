# Kuri's XP-Pen Userland drivers

This repo currently houses a command-line only utility that creates a userspace driver for the following models:
- Artist 22R Pro

It initializes the supported tablets with their default bindings but the bindings can be changed by modifying the configuration file:
`$HOME/.local/share/xp_pen_userland/driver.cfg`

Things on the TODO list:
- Add binding support for 22R Pro dials
- Make a nice GUI to perform binding changes
- Support more XP-Pen devices

## Warning
- This includes a 70-uinput-plugdev.rules file that gives users on your computer that are in the `plugdev` permission group access to uinput without SUDO. This is how I can make this driver run without having the user constantly enter their password each time.

## How to change bindings
Without a GUI changing bindings is pretty annoying.
Let's take the default binding of the top-left button on my 22R Pro:

```
{
  "XP-Pen": {
    "2331": {
      "mapping": {
        "256": [
          48
        ]
      }
    }
  }
}
```

The actual button is represented as BTN_0 which has a code of '256'. Above it is mapped to the KEY_B with a code of '48'. Let's say I want to change that binding to... CTRL+SHIFT+Z. The mapping JSON would have to be updated like the following:

```
{
  "XP-Pen": {
    "2331": {
      "mapping": {
        "256": [
          29,
          42,
          44
        ]
      }
    }
  }
}
```

Key codes:
- `29` = `KEY_LEFTCTRL`
- `42` = `KEY_LEFTSHIFT`
- `44` = `KEY_Z`

With the config file changed, sending a `kill -SIGHUP $PID` to the process (where PID is the process ID) will make it reload the config and update the bindings on the fly. After this, pressing the button will produce the expected keypresses.
