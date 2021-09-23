/*
userspace_tablet_driver_daemon
Copyright (C) 2021 - Aren Villanueva <https://github.com/kurikaesu/>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef USERSPACE_TABLET_DRIVER_DAEMON_UINPUT_PAD_ARGS_H
#define USERSPACE_TABLET_DRIVER_DAEMON_UINPUT_PAD_ARGS_H

#include <linux/uinput.h>

struct uinput_pad_args {
public:
    std::vector<int> padButtonAliases;
    bool hasWheel;
    bool hasHWheel;

    int wheelMax;
    int hWheelMax;

    unsigned short vendorId;
    unsigned short productId;
    unsigned short versionId;
    char productName[UINPUT_MAX_NAME_SIZE];
};

#endif //USERSPACE_TABLET_DRIVER_DAEMON_UINPUT_PAD_ARGS_H
