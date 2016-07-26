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

#include <getopt.h>
#include <sys/select.h>
#include <sys/time.h>

#include <chrono>
#include <iostream>
#include <limits>
#include <thread>

#include "./config.h"
#include "./exc.h"
#include "./ptrace.h"
#include "./pyframe.h"

using namespace pystack;

namespace {
const char usage_str[] = "Usage: pystack [-h|--help] [-j|--json] PID\n";

void RunOnce(pid_t pid, unsigned long addr) {
  std::vector<Frame> stack = GetStack(pid, addr);
  for (auto it = stack.rbegin(); it != stack.rend(); it++) {
    std::cout << *it << "\n";
  }
  std::cout << std::flush;
}
}  // namespace

int main(int argc, char **argv) {
  double seconds = 0;
  double sample_rate = 0.01;
  for (;;) {
    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {"rate", required_argument, 0, 'r'},
        {"seconds", required_argument, 0, 's'},
        {"version", no_argument, 0, 'v'},
        {0, 0, 0, 0}};
    int option_index = 0;
    int c = getopt_long(argc, argv, "hjr:s:v", long_options, &option_index);
    if (c == -1) {
      break;
    }
    switch (c) {
      case 0:
        if (long_options[option_index].flag != 0) {
          // if the option set a flag, do nothing
          break;
        }
        break;
      case 'h':
        std::cout << usage_str;
        return 0;
        break;
      case 'r':
        sample_rate = std::stod(optarg);
        break;
      case 's':
        seconds = std::stod(optarg);
        break;
      case 'v':
        std::cout << PACKAGE_STRING << "\n";
        return 0;
        break;
      case '?':
        // getopt_long should already have printed an error message
        break;
      default:
        abort();
    }
  }
  if (optind != argc - 1) {
    std::cerr << usage_str;
    return 1;
  }
  long pid = std::strtol(argv[argc - 1], NULL, 10);
  if (pid > std::numeric_limits<pid_t>::max() ||
      pid < std::numeric_limits<pid_t>::min()) {
    std::cerr << "PID " << pid << " is out of valid PID range.\n";
    return 1;
  }
  try {
    PtraceAttach(pid);
    const unsigned long addr = ThreadStateAddr(pid);
    const std::chrono::microseconds interval{
        static_cast<long>(sample_rate * 1000000)};
    if (seconds) {
      auto end =
          std::chrono::system_clock::now() +
          std::chrono::microseconds(static_cast<long>(seconds * 1000000));
      for (;;) {
        try {
          RunOnce(pid, addr);
        } catch (const NonFatalException &exc) {
          // continue if we get a non-fatal exception
          std::cerr << exc.what() << std::endl;
        }
        auto now = std::chrono::system_clock::now();
        if (now + interval >= end) {
          break;
        }
        PtraceDetach(pid);
        struct timespec timeout;
        timeout.tv_nsec = std::chrono::duration_cast<std::chrono::nanoseconds>(interval).count();
        pselect(0, NULL, NULL, NULL, &timeout, NULL);
        //std::this_thread::sleep_for(interval);
        std::cout << "\n";
        PtraceAttach(pid);
      }
    } else {
      RunOnce(pid, addr);
    }
  } catch (const FatalException &exc) {
    std::cerr << exc.what() << std::endl;
    return 1;
  } catch (const NonFatalException &exc) {
    std::cerr << exc.what() << std::endl;
    return 0;
  } catch (const std::exception &exc) {
    std::cerr << exc.what() << std::endl;
    return 1;
  }
  return 0;
}
