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

#include "./symbol.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <sstream>

namespace pystack {
void ELF::Close() {
  if (addr_ != NULL) {
    munmap(addr_, length_);
    addr_ = NULL;
  }
}

// mmap the file
void ELF::Open(const std::string &target) {
  Close();
  int fd = open(target.c_str(), O_RDONLY);
  if (fd == -1) {
    std::ostringstream ss;
    ss << "Failed to open target " << target << ": " << strerror(errno);
    throw FatalException(ss.str());
  }
  length_ = lseek(fd, 0, SEEK_END);
  addr_ = mmap(NULL, length_, PROT_READ, MAP_SHARED, fd, 0);
  while (close(fd) == -1) {
    ;
  }
  if (addr_ == MAP_FAILED) {
    std::ostringstream ss;
    ss << "Failed to mmap " << target << ": " << strerror(errno);
    throw FatalException(ss.str());
  }

  if (hdr()->e_ident[EI_MAG0] != ELFMAG0 ||
      hdr()->e_ident[EI_MAG1] != ELFMAG1 ||
      hdr()->e_ident[EI_MAG2] != ELFMAG2 ||
      hdr()->e_ident[EI_MAG3] != ELFMAG3) {
    std::ostringstream ss;
    ss << "File " << target << " does not have correct ELF magic header";
    throw FatalException(ss.str());
  }
  if (hdr()->e_ident[EI_CLASS] != ELFCLASS64) {
    throw FatalException("Currently only 64-bit ELF files are supported");
  }
}

void ELF::Parse() {
  // skip the first section since it must be of type SHT_NULL
  for (uint16_t i = 1; i < hdr()->e_shnum; i++) {
    const Elf64_Shdr *s = shdr(i);
    switch (s->sh_type) {
      case SHT_STRTAB:
        if (strcmp(strtab(s->sh_name), ".dynstr") == 0) {
          dynstr_ = i;
        }
        break;
      case SHT_DYNSYM:
        dynsym_ = i;
        break;
      case SHT_DYNAMIC:
        dynamic_ = i;
        break;
    }
  }
  if (dynamic_ == -1) {
    throw FatalException("Failed to find section .dynamic");
  } else if (dynstr_ == -1) {
    throw FatalException("Failed to find section .dynstr");
  } else if (dynsym_ == -1) {
    throw FatalException("Failed to find section .dynsym");
  }
}

std::vector<std::string> ELF::NeededLibs() {
  // Get all of the strings
  std::vector<std::string> needed;
  const Elf64_Shdr *s = shdr(dynamic_);
  const Elf64_Shdr *d = shdr(dynstr_);
  for (uint16_t i = 0; i < s->sh_size / s->sh_entsize; i++) {
    const Elf64_Dyn *dyn = reinterpret_cast<const Elf64_Dyn *>(
        p() + s->sh_offset + i * s->sh_entsize);
    if (dyn->d_tag == DT_NEEDED) {
      needed.push_back(
          reinterpret_cast<const char *>(p() + d->sh_offset + dyn->d_un.d_val));
    }
  }
  return needed;
}

unsigned long ELF::GetThreadState() {
  const Elf64_Shdr *s = shdr(dynsym_);
  const Elf64_Shdr *d = shdr(dynstr_);
  for (uint16_t i = 0; i < s->sh_size / s->sh_entsize; i++) {
    const Elf64_Sym *sym = reinterpret_cast<const Elf64_Sym *>(
        p() + s->sh_offset + i * s->sh_entsize);
    const char *name =
        reinterpret_cast<const char *>(p() + d->sh_offset + sym->st_name);
    if (strcmp(name, "_PyThreadState_Current") == 0) {
      return static_cast<unsigned long>(sym->st_value);
    }
  }
  return 0;
}
}  // namespace pystack
