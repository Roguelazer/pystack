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

#include "./pystring.h"

#include <cstddef>
#include <limits>

#include <Python.h>

#include "./config.h"

namespace pystack {
#if PY_MAJOR_VERSION == 2
unsigned long StringSize(unsigned long addr) {
  return addr + offsetof(PyStringObject, ob_size);
}

unsigned long StringData(unsigned long addr) {
  return addr + offsetof(PyStringObject, ob_sval);
}
#elif PY_MAJOR_VERSION == 3
unsigned long StringSize(unsigned long addr) {
  return addr + offsetof(PyVarObject, ob_size);
}

unsigned long StringData(unsigned long addr) {
  // this works only if the filename is all ascii *fingers crossed*
  return addr + sizeof(PyASCIIObject);
}
#else
static_assert(false, "Unknown Python version.");
#endif
}  // namespace pystack
