# pipe_exec

Execute ELF binaries from pipes. This makes it possible to
execute binaries that are not stored in a file system or for
which the execute permission is not set.

This makes it possible to run binaries directly from stdin;
e.g. when piping gcc output or when downloading them via
SSH.

It works by allocating an in-memory file via the
memfd_create(2) syscall, copying all data from the
executable there and finally executing it using the
fexecve(3) syscall.

pipe_exec will first try to execute the file in place; e.g.
`pexec </bin/ls` would not load ls into memory first.


## Examples

```sh
cat `which date` | pexec -u
Sun Jul  3 21:09:36 UTC 2016
```

would be equivalent to

```sh
date -u
Sun Jul  3 21:09:36 UTC 2016
```

## License and Copyright

Copyright (C) 2016 by Karolin Varner

Under CC-0 license or Public Domain as applicable.
