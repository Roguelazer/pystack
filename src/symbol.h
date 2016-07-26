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

#include <elf.h>

#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "./exc.h"

namespace pystack {

// Representation of a 64-bit ELF file.
//
// TODO: support 32-bit ELF files. One easiest way to do this would be to have
// this class templated on the architecture where the 64-bit version uses the
// Elf64_* structs and the 32-bit version uses the Elf32_* structs. Then another
// function can inspect the ELF header and tell the caller which class they
// should use.
class ELF {
 public:
  ELF() : addr_(NULL), length_(0), dynamic_(-1), dynstr_(-1), dynsym_(-1) {}
  ~ELF() { Close(); }

  // Open a file
  void Open(const std::string &target);

  // Close the file; normally the destructor will do this for you.
  void Close();

  // Parse the ELF sections.
  void Parse();

  // Find the DT_NEEDED fields. This is similar to the ldd(1) command.
  std::vector<std::string> NeededLibs();

  // Get the address of _PyThreadState_Current
  unsigned long GetThreadState();

 private:
  void *addr_;
  size_t length_;
  int dynamic_, dynstr_, dynsym_;

  inline const Elf64_Ehdr *hdr() const {
    return reinterpret_cast<const Elf64_Ehdr *>(addr_);
  }

  inline const Elf64_Shdr *shdr(int idx) const {
    if (idx < 0) {
      std::ostringstream ss;
      ss << "Illegal shdr index: " << idx;
      throw FatalException(ss.str());
    }
    return reinterpret_cast<const Elf64_Shdr *>(p() + hdr()->e_shoff +
                                                idx * hdr()->e_shentsize);
  }

  inline unsigned long p() const {
    return reinterpret_cast<unsigned long>(addr_);
  }

  inline const char *strtab(int offset) const {
    const Elf64_Shdr *strings = shdr(hdr()->e_shstrndx);
    return reinterpret_cast<const char *>(p() + strings->sh_offset + offset);
  }

  inline const char *dynstr(int offset) const {
    const Elf64_Shdr *strings = shdr(dynstr_);
    return reinterpret_cast<const char *>(p() + strings->sh_offset + offset);
  }
};
}  // namespace pystack
