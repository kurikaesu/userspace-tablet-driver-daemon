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

#include "button_mapping_configuration.h"
#include <linux/input.h>

button_mapping_configuration::button_mapping_configuration() {
    // Default constructor
}

void button_mapping_configuration::addButtonMapping(int button, int eventType, const std::vector<int>& codes) {
    button_map mapping;
    mapping.button = button;
    mapping.eventType = eventType;
    mapping.codes = codes;
    buttonMaps.push_back(mapping);
}

void button_mapping_configuration::addDialMapping(int dial, int value, int eventType, const std::vector<int>& codes) {
    dial_map mapping;
    mapping.dial = dial;
    mapping.value = value;
    mapping.eventType = eventType;
    mapping.codes = codes;
    dialMaps.push_back(mapping);
}

void button_mapping_configuration::applyToJson(nlohmann::json& config) const {
    if (!config.contains("mapping") || config["mapping"] == nullptr) {
        config["mapping"] = nlohmann::json({});
    }

    // Apply button mappings
    for (const auto& mapping : buttonMaps) {
        std::string evstring = std::to_string(mapping.eventType);
        config["mapping"]["buttons"][std::to_string(mapping.button)][evstring] = mapping.codes;
    }

    // Apply dial mappings
    for (const auto& mapping : dialMaps) {
        std::string strvalue = std::to_string(mapping.value);
        std::string evstring = std::to_string(mapping.eventType);
        config["mapping"]["dials"][std::to_string(mapping.dial)][strvalue][evstring] = mapping.codes;
    }
}

button_mapping_configuration button_mapping_configuration::createDefaultXPPenConfig() {
    button_mapping_configuration config;
    
    // Add default button mappings for XP-Pen devices
    config.addButtonMapping(BTN_0, EV_KEY, {KEY_B});
    config.addButtonMapping(BTN_1, EV_KEY, {KEY_E});
    config.addButtonMapping(BTN_2, EV_KEY, {KEY_SPACE});
    config.addButtonMapping(BTN_3, EV_KEY, {KEY_LEFTALT});
    config.addButtonMapping(BTN_4, EV_KEY, {KEY_V});
    config.addButtonMapping(BTN_5, EV_KEY, {KEY_LEFTCTRL, KEY_S});
    config.addButtonMapping(BTN_6, EV_KEY, {KEY_LEFTCTRL, KEY_Z});
    config.addButtonMapping(BTN_7, EV_KEY, {KEY_LEFTCTRL, KEY_LEFTALT, KEY_N});
    
    return config;
}

button_mapping_configuration button_mapping_configuration::createDefaultXPPenConfigWithDial() {
    button_mapping_configuration config = createDefaultXPPenConfig();
    
    // Add default dial mappings
    config.addDialMapping(REL_WHEEL, -1, EV_KEY, {KEY_LEFTCTRL, KEY_MINUS});
    config.addDialMapping(REL_WHEEL, 1, EV_KEY, {KEY_LEFTCTRL, KEY_EQUAL});
    
    return config;
}
