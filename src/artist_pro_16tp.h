/*
userspace-tablet-driver-daemon
Copyright (C) 2022 - Aren Villanueva <https://github.com/kurikaesu/>

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

#ifndef USERSPACE_TABLET_DRIVER_DAEMON_ARTIST_PRO_16TP_H
#define USERSPACE_TABLET_DRIVER_DAEMON_ARTIST_PRO_16TP_H

#include "xp_pen_unified_device.h"

class artist_pro_16tp : public xp_pen_unified_device {
public:
    artist_pro_16tp();

    std::string getProductName(int productId);
    void setConfig(nlohmann::json config);
    virtual unsigned short getDescriptorLength();
    bool handleTransferData(libusb_device_handle* handle, unsigned char* data, size_t dataLen, int productId);
};

#endif //USERSPACE_TABLET_DRIVER_DAEMON_ARTIST_PRO_16TP_H
