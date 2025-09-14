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

#ifndef USERSPACE_TABLET_DRIVER_DAEMON_DEVICE_SPECIFICATION_H
#define USERSPACE_TABLET_DRIVER_DAEMON_DEVICE_SPECIFICATION_H

#include <string>
#include <map>
#include <vector>

struct device_specification {
    // Product information
    std::map<int, std::string> productNames;
    std::vector<int> supportedProductIds;
    
    // Device capabilities
    int numButtons;
    bool hasDial;
    bool hasHorizontalDial;
    
    // Frame event handling
    int buttonByteIndex;
    int dialByteIndex;
    
    // Constructor with default values
    device_specification() : 
        numButtons(0),
        hasDial(false),
        hasHorizontalDial(false),
        buttonByteIndex(2),
        dialByteIndex(7) {}
    
    // Add a product ID and name
    void addProduct(int productId, const std::string& name) {
        productNames[productId] = name;
        supportedProductIds.push_back(productId);
    }
};

#endif //USERSPACE_TABLET_DRIVER_DAEMON_DEVICE_SPECIFICATION_H
