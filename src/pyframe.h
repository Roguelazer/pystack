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

#include <iostream>
#include <string>
#include <vector>

namespace pystack {

class Frame {
 public:
  Frame() = delete;
  Frame(const Frame &other) : file_(other.file_), line_(other.line_) {}
  Frame(const std::string &file, size_t line) : file_(file), line_(line) {}

  inline const std::string &file() const { return file_; }
  inline size_t line() const { return line_; }

 private:
  const std::string file_;
  const size_t line_;
};

std::ostream &operator<<(std::ostream &os, const Frame &frame);

// Locate _PyThreadState_Current
unsigned long ThreadStateAddr(pid_t pid);

// Get the stack. The stack will be in reverse order (most recent frame first).
std::vector<Frame> GetStack(pid_t pid, unsigned long addr);
}  // namespace pystack
