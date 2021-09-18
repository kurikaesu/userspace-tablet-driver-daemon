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

#include <iostream>
#include "unix_socket_message_queue.h"

unix_socket_message_queue::unix_socket_message_queue() {
    messages[message_destination::driver] = std::map<short, std::vector<unix_socket_message*> >();
    messages[message_destination::gui] = std::map<short, std::vector<unix_socket_message*> >();
}

unix_socket_message_queue::~unix_socket_message_queue() {

}

void unix_socket_message_queue::addMessage(unix_socket_message *message) {
    if (message == nullptr) {
        return;
    }

    auto record = messages[message->destination].find(message->vendor);
    if (record == messages[message->destination].end()) {
        messages[message->destination][message->vendor] = std::vector<unix_socket_message*>();
    }

    messages[message->destination][message->vendor].push_back(message);
}

std::vector<unix_socket_message*> unix_socket_message_queue::getMessagesFor(message_destination destination, short vendor) {
    auto record = messages[destination].find(vendor);
    if (record != messages[destination].end() && record->second.size() > 0) {
        auto returnedItems = std::vector<unix_socket_message*>(record->second);
        record->second.clear();
        return returnedItems;
    }

    return std::vector<unix_socket_message*>();
}

std::vector<unix_socket_message*> unix_socket_message_queue::getResponses() {
    auto returnedItems = std::vector<unix_socket_message*>();
    for (auto it = messages[message_destination::gui].begin(); it != messages[message_destination::gui].end(); ++it) {
        returnedItems.insert(returnedItems.end(), it->second.begin(), it->second.end());
    }

    messages[message_destination::gui].clear();

    return returnedItems;
}
