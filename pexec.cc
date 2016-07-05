#include <unistd.h>
#include <sys/syscall.h>
#include <linux/memfd.h>

#include <cstring>

#include <string>
#include <sstream>
#include <vector>
#include <iostream>

#define UNUSED __attribute__ ((unused))

/// Represents an error condition with a dynamic error
/// message
struct err : std::exception {
  std::string _what;

  err(const std::string &msg, int errnum) {
    std::stringstream ss;
    ss << msg << " ERROR " << errnum << ": " << strerror(errnum);
    _what = ss.str();
  }

  err(const std::string &msg) : _what(msg) {}
  err(int errnum) : err("", errnum) {}

  const char* what() const noexcept {
    return _what.c_str();
  }
};

/// Strict file descriptor; auto closes the fd when it goes
/// out of scope
struct xfd {
  int fd;

  xfd(int fd) : fd(fd) {}
  ~xfd() {
    close(fd);
  }
};

template<typename V=int>
V ckerrno(V r, const std::string &msg) {
  if (r < 0) throw err(msg, errno);
  return r;
}

/// The x... family of calls correspond to their
/// syscall/glibc api, but all of them are checked; they
/// will throw exceptions when an error ocurrs
void xclose(int fd) {
  ckerrno(close(fd), "close()");
}

size_t xread(int fd, void *buf, size_t count) {
  auto r = read(fd, buf, count);
  if (r == -1 && errno == EINTR)
    return xread(fd, buf, count);
  else
    return ckerrno(r, "read()");
}

size_t xwrite(int fd, void *buf, size_t count) {
  auto r = write(fd, buf, count);
  if (r == -1 && errno == EINTR)
    return xwrite(fd, buf, count);
  else
    return ckerrno(r, "write()");
}

void xfexecve(int fd, char *const argv[], char *const envp[]) {
  ckerrno(fexecve(fd, argv, envp), "fexecve()");
  throw err("fexecve() ERROR: Should have terminated cexec");
}

xfd xmemfd_create(const char *name, unsigned int flags=0) {
  int r = syscall(SYS_memfd_create, name, flags);
  ckerrno(r, "memfd_create()");
  return xfd(r);
}

/// Like xwrite but makes sure really all data is written or
/// an error is thrown; xwrite may succeed only having
/// written some of the data.
void xwrite_all(int fd, void *buf, size_t count) {
  size_t written = 0, iter=0;
  for (; written < count && iter < 2000; iter++)
    written += xwrite(fd, (char*)buf+written, count - written);

  if (iter > 2000)
    throw("write() ERROR Giving up trying zero bytes lots of times");
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
    buf_len = xread(fdin, &buf[0], buf.size());
    if (buf_len == 0) break; // EOF
    xwrite_all(fdout, &buf[0], buf_len);
  }
}

template<typename F>
bool drop_exceptions(F f) {
  try {
    f();
    return true;
  } catch (std::exception &e) {
    return false;
  }
}

template<typename F>
bool print_exceptions(F f) {
  try {
    f();
    return true;
  } catch (std::exception &e) {
    std::cerr << "[ERROR] " << e.what() << std::endl;
    return false;
  }
}

template<typename F>
void fatal_exceptions(F f) {
  if (!print_exceptions(f)) exit(1);
}

int main(int argc UNUSED, char *argv[]) {
  fatal_exceptions([&]() {

    drop_exceptions([&]() {
      xfexecve(0, argv, __environ);
    });

    const xfd vfd = xmemfd_create("Virtual file", MFD_CLOEXEC);
    transfer(0, vfd.fd);
    xfexecve(vfd.fd, argv, __environ);

  });
}
