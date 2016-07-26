// This file is part of Pystack.
//
// Pystack is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Pystack is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Pystack.  If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <sys/types.h>

#include <string>

namespace pystack {
// Find libpython2.7.so and its offset for an ASLR process.
size_t LocateLibPython(pid_t pid, const std::string &hint, std::string *path);
}  // namespace pystack
