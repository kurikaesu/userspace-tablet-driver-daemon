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

#ifndef USERSPACE_TABLET_DRIVER_DAEMON_BUTTON_MAPPING_CONFIGURATION_H
#define USERSPACE_TABLET_DRIVER_DAEMON_BUTTON_MAPPING_CONFIGURATION_H

#include <vector>
#include <string>
#include "includes/json.hpp"
#include <linux/input.h>

class button_mapping_configuration {
public:
    struct button_map {
        int button;
        int eventType;
        std::vector<int> codes;
    };
    
    struct dial_map {
        int dial;
        int value;
        int eventType;
        std::vector<int> codes;
    };
    
    button_mapping_configuration();
    
    // Add a button mapping
    void addButtonMapping(int button, int eventType, const std::vector<int>& codes);
    
    // Add a dial mapping
    void addDialMapping(int dial, int value, int eventType, const std::vector<int>& codes);
    
    // Apply the configuration to a JSON object
    void applyToJson(nlohmann::json& config) const;
    
    // Create a default configuration for XP-Pen devices
    static button_mapping_configuration createDefaultXPPenConfig();
    
    // Create a default configuration with dial mappings for XP-Pen devices
    static button_mapping_configuration createDefaultXPPenConfigWithDial();
    
private:
    std::vector<button_map> buttonMaps;
    std::vector<dial_map> dialMaps;
};

#endif //USERSPACE_TABLET_DRIVER_DAEMON_BUTTON_MAPPING_CONFIGURATION_H
