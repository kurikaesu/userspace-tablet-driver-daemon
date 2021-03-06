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

#ifndef USERSPACE_TABLET_DRIVER_DAEMON_EVENT_HANDLER_H
#define USERSPACE_TABLET_DRIVER_DAEMON_EVENT_HANDLER_H


#include <libusb-1.0/libusb.h>
#include <map>
#include <deque>
#include <fstream>
#include "vendor_handler.h"
#include "usb_devices.h"
#include "hotplug_event.h"
#include "includes/json.hpp"
#include "socket_server.h"

class event_handler {
public:
    event_handler();
    ~event_handler();
    int run();

private:
    static void sigHandler(int signo);
    static int hotplugCallback(struct libusb_context* context, struct libusb_device* device,
                                       libusb_hotplug_event event, void* user_data);

    void addHandler(vendor_handler* handler);

    std::string getConfigLocation();
    std::string getConfigFileLocation();
    void loadConfiguration();
    void saveConfiguration();

    void handleMessages();

    static bool running;
    static event_handler* instance;

    std::map<short, vendor_handler*> vendorHandlers;
    usb_devices *devices;

    std::deque<hotplug_event> hotplugEvents;

    // Config related
    nlohmann::json driverConfigJson;

    socket_server socketServer;
    unix_socket_message_queue messageQueue;
};


#endif //USERSPACE_TABLET_DRIVER_DAEMON_EVENT_HANDLER_H
