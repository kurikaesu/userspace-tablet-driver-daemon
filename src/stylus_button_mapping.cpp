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

#include <linux/input.h>
#include "stylus_button_mapping.h"

stylus_button_mapping::stylus_button_mapping() {

}

std::vector<aliased_input_event> stylus_button_mapping::getStylusButtonMap(int eventCode) {
    auto record = eventStylusButtonMap.find(eventCode);
    if (record != eventStylusButtonMap.end()) {
        return eventStylusButtonMap.at(eventCode);
    }

    std::vector<aliased_input_event> temp;

    return temp;
}

void stylus_button_mapping::setStylusButtonMap(int eventCode, const std::vector<aliased_input_event> &events) {
    eventStylusButtonMap[eventCode] = events;
}
