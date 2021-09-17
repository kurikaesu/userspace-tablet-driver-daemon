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

#ifndef XP_PEN_USERLAND_UNIX_SOCKET_MESSAGE_QUEUE_H
#define XP_PEN_USERLAND_UNIX_SOCKET_MESSAGE_QUEUE_H


#include "unix_socket_message.h"

class unix_socket_message_queue {
public:
    unix_socket_message_queue();
    ~unix_socket_message_queue();

    void addMessage(unix_socket_message* message);
};


#endif //XP_PEN_USERLAND_UNIX_SOCKET_MESSAGE_QUEUE_H
