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

#ifndef USERSPACE_TABLET_DRIVER_DAEMON_USB_DEVICES_H
#define USERSPACE_TABLET_DRIVER_DAEMON_USB_DEVICES_H

#include <libusb-1.0/libusb.h>
#include <map>
#include "vendor_handler.h"

class usb_devices {
public:
    usb_devices();

    libusb_context* getContext();

    void handleEvents();

    std::map<short, std::vector<short> > getCandidateDevices(const std::map<short, vendor_handler*> vendorHandlers);
    void handleDeviceAttach(const std::map<short, vendor_handler*> vendorHandlers, struct libusb_device* device);
    void handleDeviceDetach(const std::map<short, vendor_handler *> vendorHandlers, struct libusb_device* device);
private:
    libusb_context *context = NULL;
    libusb_device **lusb_list = NULL;
};


#endif //USERSPACE_TABLET_DRIVER_DAEMON_USB_DEVICES_H
