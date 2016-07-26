# Pystack

Pystack is a program that can print the current stack trace for an arbitrary
running Python process. It is a little like Python's
[`traceback.print_tb()`](https://docs.python.org/2/library/traceback.html#traceback.print_tb),
but rather than being a Python function it is a CLI tool that you can run
against arbitrary processes that weren't already instrumented to dump their
stack. There's a
[blog post explaining how it works](https://eklitzke.org/pystack).

You use it like this:

    pystack <PID>

If everything goes correctly, you'll see the stack trace printed to stdout:

    $ pystack 15776
    ./blog/env/bin/blog-generate:9
    ./blog/blog/app.py:27
    ./blog/blog/generate.py:53
    ./blog/blog/generate.py:61
    ./blog/blog/parser.py:101
    ./blog/env/lib/python2.7/site-packages/markdown/__init__.py:371
    ./blog/env/lib/python2.7/site-packages/markdown/blockparser.py:65
    ./blog/env/lib/python2.7/site-packages/markdown/blockparser.py:80
    ./blog/env/lib/python2.7/site-packages/markdown/blockparser.py:97
    ./blog/env/lib/python2.7/site-packages/markdown/blockprocessors.py:429

The output is ordered according to the same convention as a Python "backtrace",
i.e. such that the most recently executed line is on the bottom of the output
and the least recently executed line is on the top of the output.

Pystack is implemented using the magic of the
[`ptrace(2)`](http://man7.org/linux/man-pages/man2/ptrace.2.html) system call.
Conceptually it works in a similar way to GDB. Pystack will attach to an
arbitrary PID and then use the magic of ptrace to peer into the Python
interpreter's memory and extract the current stack trace. See the "How Does It
Work?" section below for more details.

I wrote this software because I was frustrated with the existing profiling and
debugging tools available for Python. I believe that this kind of software can
be useful as the basis of high quality, low overhead profiling tools. See
"Advanced Usage" below.

## What Platforms Does It Work On?

Currently Pystack only works on 64-bit Linux systems. It should be relatively
easy to port to 32-bit systems and BSD.

In principle it can be made to work on OS X. The hardest part would be extracing
symbols out of the Python binary, as OS X uses the Mach-O executable format
rather than ELF. If you have a non-hacky way to extract symbols on OS X send me
a pull request.

Generally there are two ways to compile the Python interpreter. In the default
compilation mode you get a "static" binary that has the Python symbols built in.
If instead Python was compiled with the `--enable-shared` option you get a
"dynamic" binary that links against libpython, and libpython has the actual
interpreter symbols. The "static" mode is the default and what is also shipped
by most Linux distributions. The "dynamic" mode is used by Fedora (and possibly
other distributions). Pystack can detect how the Python interpreter was built
and supports both use cases. You can also use Pystack with processes that embed
Python, e.g. [uWSGI](https://uwsgi-docs.readthedocs.io/en/latest/).

## Compiling

You'll need the following:

 * A C++ compiler with C++11 support
 * Autotools (autoconf + automake)
 * Python headers

Then in the root of the project run:

    ./autogen.sh

This will create the `./configure` file. You can then proceed with the build as
usual:

    ./configure
    make
    make install

This invocation should install the correct build dependencies on Fedora:

    sudo dnf install autoconf automake gcc-c++ python-devel

This invocation should install the correct build dependencies on Debian/Ubuntu:

    sudo apt-get install autoconf build-essential pkg-config python-dev

### I'm Young and Hip and Want To Use Python 3

That's supported! Compile Pystack like this:

    ./configure --with-python=python3

If you have file names that contain non-ASCII Unicode code points you may get
incorrect output. Pull requests to improve Unicode handling here are very
welcome.

## How Does It Work?

As already mentioned, Pystack uses the `ptrace(2)` system call to read a remote
process's memory image. It works roughly like this:

 * attach to the process using `PTRACE_ATTACH`
 * read and decode the ELF executable for the process
 * based on what is read from the ELF, determine if this is a static or dynamic
   Python build
 * locate the `_PyThreadState_Current` symbol (which will either be in the
   Python interpreter, or in libpython, depending on the interpreter build mode)
   * if the symbol exists in libpython, find the
     [ASLR](https://en.wikipedia.org/wiki/Address_space_layout_randomization)
     offset for libpython
 * locate the current frame object from `_PyThreadState_Current` and then
   recursively use the `PTRACE_PEEKDATA` command to read stack frames and decode
   their fields

Everything but the last step is setup work. Therefore Pystack implements a
mechanism to "monitor" a process and get repeated dumps. In the monitoring mode
the setup work is done only once, and then Pystack repeatedly attaches and dumps
the process at a given frequency. When monitoring a process in such a way the
process can be queried at a very high sample rate, which is useful for
profiling. You use the monitoring mode like this:

    pystack -s 5 -r 0.001 4282

This would sample PID 4282 for 5 seconds waiting 1 millisecond (i.e. 0.001
seconds) between each sample.

## Advanced Usage

You can use Pystack to build a Python profiler of your design. It's fun and
easy!

Normal Python profilers like
[profile and cProfile](https://docs.python.org/2/library/profile.html) work by
using the
[`sys.settrace()`](https://docs.python.org/2/library/sys.html#sys.settrace)
routine. This lets you register a callback that the Python interpreter runs very
frequently. This is nice because the profiling function is run very often and at
all of the interesting points in your program which yields good data. However
this approach also has very high overhead.

You can also build a high-resolution signal based timer using
[`signal.setitimer()`](https://docs.python.org/2/library/signal.html#signal.setitimer),
and this is what a number of Python projects actually do. However, you need to
have your process already instrumented to do this, and the overhead can be high
if the signal handler is also Python code.

Pystack has the following nice properties

 * you can run it on any process, without having planned to use it beforehand
 * you can run it any granularity you find useful, whether that's very fast
   (e.g. microsecond granulariy) or very slow (e.g. second granularity)
 * it's implemented in C++ with an eye for efficiency, so it's very fast and the
   pause times are low

The monitoring mode is described in the previous section ("How Does It Work?").
There's a tradeoff to be made here between sampling frequency and overhead:
higher sampling rates will get more accurate data, but at the cost of higher
overhead. Building a profiler based on Pystack is left as an exercise to the
reader.

## Troubleshooting

This section explains some of the more common error messages you might see.

### "No active frame for the Python interpreter."

You may see this error message from time to time. What does it mean?

Interestingly the Python interpreter does not always have an active frame. For
instance, if you take an idle Python REPL and attach it with GDB, you'll get a
stack trace somewhat like this:

    (gdb) bt
    #0  0x00007ff1d8a3c0e3 in __select_nocancel () at ../sysdeps/unix/syscall-template.S:84
    #1  0x00007ff1d1bc6908 in call_readline () from /usr/lib64/python2.7/lib-dynload/readline.so
    #2  0x00007ff1d9675200 in PyOS_Readline () from /lib64/libpython2.7.so.1.0
    #3  0x00007ff1d9675ed7 in tok_nextc () from /lib64/libpython2.7.so.1.0
    #4  0x00007ff1d9676d28 in PyTokenizer_Get () from /lib64/libpython2.7.so.1.0
    #5  0x00007ff1d9672b0f in parsetok () from /lib64/libpython2.7.so.1.0
    #6  0x00007ff1d9732162 in PyParser_ASTFromFile () from /lib64/libpython2.7.so.1.0
    #7  0x00007ff1d97330ca in PyRun_InteractiveOneFlags () from /lib64/libpython2.7.so.1.0
    #8  0x00007ff1d973330e in PyRun_InteractiveLoopFlags () from /lib64/libpython2.7.so.1.0
    #9  0x00007ff1d973398e in PyRun_AnyFileExFlags () from /lib64/libpython2.7.so.1.0
    #10 0x00007ff1d97454a0 in Py_Main () from /lib64/libpython2.7.so.1.0
    #11 0x00007ff1d8963731 in __libc_start_main (main=0x558ff2f927b0 <main>, argc=1, argv=0x7ffd709e7a08, init=<optimized out>,
        fini=<optimized out>, rtld_fini=<optimized out>, stack_end=0x7ffd709e79f8) at ../csu/libc-start.c:289
    #12 0x0000558ff2f927e9 in _start ()

None of the functions in this backtrace are evaluating a frame object. By
contrast, if you look at the stack trace for a Python process doing real work
you'll typically see `PyEval_EvalFrameEx` somewhere in the stack trace, which is
the function that typically evaluates a Python frame object. In this situation
the tail-most instance of `PyEval_EvalFrameEx` will be evaluating the "current"
frame.

Concretely, we say there is no active frame when `_PyThreadState_Current` is a
null pointer.

One simple way to reproduce this issue is to put a line of code like this in a
Python file you are tracing:

```python
import select
select.select([], [], [])
```

This will cause the Python process to hang forever, but `_PyThreadState_Current`
will be a null pointer and thus there will be no active frame.

This also arises in embedding contexts. If you look at a uWSGI process that is
currently serving a request it will have an active frame, but a uWSGI process
that is just idle and waiting for traffic will not have an active frame.

### Failed to PTRACE_PEEKDATA at 0x2b: Input/output error

If you consistently get an error like this the most likely explanation is that
you built against Python 2 but are trying to trace a Python 3 program (or vice
versa). The structure offsets for Python 2 and Python 3 are different, so if you
have a mismatched build Pystack will get confused.

To build against Python 2:

    ./configure

To build against Python 3:

    ./configure --python=python3

**TODO:** be better at auto-detecting the appropriate Python to build against,
and also warn when the target appears to be mismatched.
