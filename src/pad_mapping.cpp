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

#include "pad_mapping.h"

pad_mapping::pad_mapping() {

}

std::vector<int> pad_mapping::getPadMap(int eventCode) {
    auto record = eventPadMap.find(eventCode);
    if (record != eventPadMap.end()) {
        return eventPadMap.at(eventCode);
    }

    std::vector<int> temp;
    temp.push_back(eventCode);

    return temp;
}

void pad_mapping::setPadMap(int eventCode, const std::vector<int> &events) {
    eventPadMap[eventCode] = events;
}
