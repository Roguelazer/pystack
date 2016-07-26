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

#include "./aslr.h"
#include "./exc.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace pystack {
// Find libpython2.7.so and its offset for an ASLR process
size_t LocateLibPython(pid_t pid, const std::string &hint, std::string *path) {
  std::ostringstream ss;
  ss << "/proc/" << pid << "/maps";
  std::ifstream fp(ss.str());
  std::string line;
  std::string elf_path;
  while (std::getline(fp, line)) {
    if (line.find(hint) != std::string::npos &&
        line.find(" r-xp ") != std::string::npos) {
      size_t pos = line.find('/');
      if (pos == std::string::npos) {
        throw FatalException("Did not find libpython absolute path");
      }
      *path = line.substr(pos);
      pos = line.find('-');
      if (pos == std::string::npos) {
        throw FatalException("Did not find libpython virtual memory address");
      }
      return std::strtol(line.substr(0, pos).c_str(), nullptr, 16);
    }
  }
  return 0;
}
}  // namespace pystack
