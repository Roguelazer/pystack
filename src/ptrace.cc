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

#include "./ptrace.h"

#include <cstring>
#include <sstream>
#include <stdexcept>
#include <utility>

#include <sys/ptrace.h>
#include <sys/wait.h>

#include "./exc.h"

namespace pystack {
void PtraceAttach(pid_t pid) {
  if (ptrace(PTRACE_ATTACH, pid, 0, 0)) {
    std::ostringstream ss;
    ss << "Failed to attach to PID " << pid << ": " << strerror(errno);
    throw FatalException(ss.str());
  }
  if (wait(nullptr) == -1) {
    std::ostringstream ss;
    ss << "Failed to wait on PID " << pid << ": " << strerror(errno);
    throw FatalException(ss.str());
  }
}

void PtraceDetach(pid_t pid) {
  if (ptrace(PTRACE_DETACH, pid, 0, 0)) {
    std::ostringstream ss;
    ss << "Failed to detach PID " << pid << ": " << strerror(errno);
    throw FatalException(ss.str());
  }
}

long PtracePeek(pid_t pid, unsigned long addr) {
  errno = 0;
  const long data = ptrace(PTRACE_PEEKDATA, pid, addr, 0);
  if (data == -1 && errno != 0) {
    std::ostringstream ss;
    ss << "Failed to PTRACE_PEEKDATA at " << reinterpret_cast<void *>(addr)
       << ": " << strerror(errno);
    throw FatalException(ss.str());
  }
  return data;
}

std::string PtracePeekString(pid_t pid, unsigned long addr) {
  std::ostringstream dump;
  unsigned long off = 0;
  while (true) {
    const long val = PtracePeek(pid, addr + off);

    // XXX: this can be micro-optimized, c.f.
    // https://graphics.stanford.edu/~seander/bithacks.html#ZeroInWord
    const std::string chunk(reinterpret_cast<const char *>(&val), sizeof(val));
    dump << chunk.c_str();
    if (chunk.find_first_of('\0') != std::string::npos) {
      break;
    }
    off += sizeof(val);
  }
  return dump.str();
}

std::unique_ptr<uint8_t[]> PtracePeekBytes(pid_t pid, unsigned long addr,
                                           size_t nbytes) {
  // align the buffer to a word size
  if (nbytes % sizeof(long)) {
    nbytes = (nbytes / sizeof(long) + 1) * sizeof(long);
  }
  std::unique_ptr<uint8_t[]> bytes(new uint8_t[nbytes]);

  size_t off = 0;
  while (off < nbytes) {
    const long val = PtracePeek(pid, addr + off);
    memmove(bytes.get() + off, &val, sizeof(val));
    off += sizeof(val);
  }
  return std::move(bytes);
}
}  // namespace pystack
