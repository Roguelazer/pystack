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
#include <unistd.h>

#include <memory>
#include <string>

namespace pystack {
// attach to a process
void PtraceAttach(pid_t pid);

// detach a process
void PtraceDetach(pid_t pid);

// read the long word at an address
long PtracePeek(pid_t pid, unsigned long addr);

// peek a null-terminated string
std::string PtracePeekString(pid_t pid, unsigned long addr);

// peek some number of bytes
std::unique_ptr<uint8_t[]> PtracePeekBytes(pid_t pid, unsigned long addr,
                                           size_t nbytes);
}  // namespace pystack
