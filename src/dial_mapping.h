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

#ifndef XP_PEN_USERLAND_DIAL_MAPPING_H
#define XP_PEN_USERLAND_DIAL_MAPPING_H

#include <vector>
#include <map>
#include <string>
#include "aliased_input_event.h"

class dial_mapping {
public:
    dial_mapping();

    std::vector<aliased_input_event> getDialMap(int eventCode, int value, int data);
    void setDialMap(int eventCode, std::string value, const std::vector<aliased_input_event> &events);
private:
    std::map<int, std::map<std::string, std::vector<aliased_input_event> > > eventDialMap;
};


#endif //XP_PEN_USERLAND_DIAL_MAPPING_H
