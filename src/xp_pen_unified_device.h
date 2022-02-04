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

#ifndef USERSPACE_TABLET_DRIVER_DAEMON_XP_PEN_UNIFIED_DEVICE_H
#define USERSPACE_TABLET_DRIVER_DAEMON_XP_PEN_UNIFIED_DEVICE_H

#include "transfer_handler.h"

class xp_pen_unified_device : public transfer_handler {
protected:
    virtual int sendInitKeyOnInterface();
    virtual bool attachToInterfaceId(int interfaceId);
    virtual unsigned short getDescriptorLength();
    bool attachDevice(libusb_device_handle* handle, int interfaceId, int productId);
    void handleDigitizerEvent(libusb_device_handle* handle, unsigned char* data, size_t dataLen, int offsetPressure = 0);
    virtual std::string getInitKey() override;
};


#endif //USERSPACE_TABLET_DRIVER_DAEMON_XP_PEN_UNIFIED_DEVICE_H
