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

#include "dial_mapping.h"

dial_mapping::dial_mapping() {

}

std::vector<aliased_input_event> dial_mapping::getDialMap(int eventCode, int value, int data) {
    auto record = eventDialMap.find(value);
    if (record != eventDialMap.end()) {
        std::string strvalue = std::to_string(data);
        auto value = record->second.find(strvalue);
        if (value != record->second.end()) {
            return value->second;
        }
    }

    std::vector<aliased_input_event> temp;
    aliased_input_event tempEvent {
        eventCode, value, data
    };
    temp.push_back(tempEvent);

    return temp;
}

void dial_mapping::setDialMap(int eventCode, std::string value, const std::vector<aliased_input_event> &events) {
    eventDialMap[eventCode] = std::map<std::string, std::vector<aliased_input_event> >();
    eventDialMap[eventCode][value] = events;
}
