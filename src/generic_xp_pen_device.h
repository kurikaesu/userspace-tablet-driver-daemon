/*
userspace-tablet-driver-daemon
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

#ifndef USERSPACE_TABLET_DRIVER_DAEMON_GENERIC_XP_PEN_DEVICE_H
#define USERSPACE_TABLET_DRIVER_DAEMON_GENERIC_XP_PEN_DEVICE_H

#include "xp_pen_unified_device.h"

class generic_xp_pen_device : public xp_pen_unified_device {
public:
    generic_xp_pen_device(int productId);

    std::string getProductName(int productId);
    void setConfig(nlohmann::json config);
    bool handleTransferData(libusb_device_handle* handle, unsigned char* data, size_t dataLen);
private:
    void handleFrameEvent(libusb_device_handle* handle, unsigned char* data, size_t dataLen);
};


#endif //USERSPACE_TABLET_DRIVER_DAEMON_GENERIC_XP_PEN_DEVICE_H
