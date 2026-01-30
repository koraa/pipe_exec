# pipe_exec

Execute ELF binaries from pipes & and terminals. This makes it possible to
execute binaries that are not stored in a file system or for
which the execute permission is not set and to run binaries
directly from stdin; e.g. when piping gcc output or when
downloading them via SSH.

It works by allocating an in-memory file via the
memfd_create(2) syscall, copying all data from the
executable there and finally executing it using the
fexecve(3) syscall.

pipe_exec will first try to execute the file in place; e.g.
`pexec </bin/ls` would not load ls into memory first.


## Usage

```sh
$ cat `which date` | pexec -u
Sun Jul  3 21:09:36 UTC 2016
```

## Installation

Install the required packages:

- build-essential

And then compile the code:

```sh
make
```

To install copy the binaries to /usr/local/bin or execute this command
which will perform the copy for you.

```sh
sudo make install
```

You can supply `$PREFIX` to use a different installation directory; the
default prefix is `/usr/local.`

## Maintenance log

This project is stable, very simple software. As such, frequent development is not necessary for stability.
I check the health of this project occasionally; in case of doubt you can always check in in the issues.

- 2024-06-26 – Everything working as it should :)
- 2026-01-30 – Merged PR [#6](https://github.com/koraa/pipe_exec/pull/6) to use install command for installation. Everything working as it should.

## Help Wanted

- Create a Nix flake for us
- Rewrite in Rust?
- Package manager support (esp. AUR, Nixpkgs)

## License and Copyright

Copyright (C) 2016-2026 by Karolin Varner

MIT License

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
