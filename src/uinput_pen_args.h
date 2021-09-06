/*
xp_pen_userland
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

#ifndef XP_PEN_USERLAND_UINPUT_PEN_ARGS_H
#define XP_PEN_USERLAND_UINPUT_PEN_ARGS_H

#include <linux/uinput.h>

struct uinput_pen_args {
public:
    int maxWidth;
    int maxHeight;
    int maxPressure;

    int maxTiltX;
    int maxTiltY;

    unsigned short vendorId;
    unsigned short productId;
    unsigned short versionId;
    char productName[UINPUT_MAX_NAME_SIZE];
};

#endif //XP_PEN_USERLAND_UINPUT_PEN_ARGS_H
