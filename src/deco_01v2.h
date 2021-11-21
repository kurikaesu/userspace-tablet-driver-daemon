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

#ifndef USERSPACE_TABLET_DRIVER_DAEMON_DECO_01V2_H
#define USERSPACE_TABLET_DRIVER_DAEMON_DECO_01V2_H


#include "deco.h"

class deco_01v2 : public deco {
public:
    deco_01v2();

    std::string getProductName(int productId);
};


#endif //USERSPACE_TABLET_DRIVER_DAEMON_DECO_01V2_H
