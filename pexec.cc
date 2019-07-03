extern "C" {
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <linux/memfd.h>
}

#include <vector>

namespace pexec {

template<typename T=int>
T cksys(const char *msg, T r) {
  if (r >= 0) return r;
  fprintf(stderr, "Fatal Error in %s: %s", msg, strerror(r));
  _exit(1);
}

void close(int fd) {
  cksys("close()", ::close(fd));
}

size_t read(int fd, void *buf, size_t count) {
  if (count == 0) return 0;
  while (true) {
    auto r = ::read(fd, buf, count);
    if (r < 0 && errno == EINTR) continue;
    return cksys("read()", r);
  }
}

size_t write(int fd, void *buf, size_t count) {
  if (count == 0) return 0;
  while (true) {
    auto r = ::write(fd, buf, count);
    if (r < 0 && errno == EINTR) continue;
    return cksys("write()", r);
  }
}

/// Like write but makes sure really all data is written or
/// an error is thrown; write may succeed only having
/// written some of the data.
void write_all(int fd, void *buf, size_t count) {
  for (size_t written = 0; written < count; )
    written += write(fd, (char*)buf+written, count - written);
}

/// Stream the contents of one fd into another
///
/// ARGS
///   fdin  – The file descriptor to read from
///   fdout – The file descriptor to write into
///
/// RETURN 1 shall be returned if the contents of fdin where
///     successfully transferred to fdout; otherwise -1 will
///     be returned and errno shall be set accordingly
void transfer(int fdin, int fdout) {
  // NOTE: This is pretty inefficient we need to transfer
  // the data between kernel space and user space and we do
  // multiple syscalls.
  // Optimally we should just use a single sendfile syscall
  // (which would not transfer any data and it would in the
  // best case even be zero copy).
  // Unfortunately, sendfile does not yet support O_APPEND,
  // so we can not just use that. We need to ftruncate the
  // virtual file to a decent size before we can write
  // there; if we can not find out the real size of stdin we
  // need to do this in chunks. And then we still need to
  // fall back to using read+write if sendfile fails for
  // undefined reasons, so this is not quite
  // straightforward.
  // IS THERE A LIBRARY FOR THIS?

  std::vector<unsigned char> buf;
  buf.resize(1024*1024*2); // 2Mb

  size_t buf_len = 1;

  while (1) {
    buf_len = read(fdin, &buf[0], buf.size());
    if (buf_len == 0) break; // EOF
    write_all(fdout, &buf[0], buf_len);
  }
}

extern "C" int main(int, char *argv[]) {
  // Try executing stdin in place; if this works, execution
  // of this program will terminate, so we can assume that some
  // error occurred if the program keeps going
  ::fexecve(0, argv, __environ);

  // OK; it's probably a file; copy into a anonymous, memory backed
  // temp file, then it should work
  const auto f = cksys("memfd_create()", ::syscall(SYS_memfd_create, "Virtual File", MFD_CLOEXEC));
  transfer(0, f);
  cksys("fexecve()", ::fexecve(f, argv, __environ));
  fprintf(stderr, "Fatal Error in fexecve(): Should have terminated the process");
  return 1;
}

}
