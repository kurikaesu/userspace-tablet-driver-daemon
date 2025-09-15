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

#ifndef USERSPACE_TABLET_DRIVER_DAEMON_STAR_G640_H
#define USERSPACE_TABLET_DRIVER_DAEMON_STAR_G640_H


#include "star.h"

class star_g640 : public star {
public:
    star_g640();

    // Override only the methods that need custom behavior
    std::string getProductName(int productId) override;
    void setConfig(nlohmann::json config) override;
    bool attachDevice(libusb_device_handle *handle, int interfaceId, int productId) override;
    std::string getInitKey() override;
};


#endif //USERSPACE_TABLET_DRIVER_DAEMON_STAR_G640_H
