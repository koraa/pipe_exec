#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <linux/memfd.h>

ssize_t cksys(const char *msg, ssize_t r) {
  if (r >= 0) return r;
  fprintf(stderr, "Fatal Error in %s: %s\n", msg, strerror(errno));
  _exit(1);
}

void safe_ftruncate(int fd, off_t len) {
  while (true) {
    int r = ftruncate(fd, len);
    if (r == -1 && errno == EINTR) continue;
    cksys("ftruncate()", r);
    return;
  }
}

void transfer_userspace(int fdin, int fdout) {
  size_t off=0, avail=1024*1024*2; // 2MB
  // Allocate space in the memfd and map it into our userspace memory
  safe_ftruncate(fdout, avail); // We know fdout is a memfd
  char *buf = (char*)mmap(NULL, avail, PROT_WRITE, MAP_SHARED, fdout, 0);
  cksys("mmap()", (ssize_t)buf);

  while (true) {
    // We ran out of space - double the size of the buffer and
    // remap it into memory
    if (off == avail) {
      const size_t nu = avail*2;
      safe_ftruncate(fdout, nu); // We know fdout is a memfd
      buf = mremap(buf, avail, nu, MREMAP_MAYMOVE);
      cksys("mremap()", (ssize_t)buf);
      avail = nu;
    }

    // Write data directly to the mapped buffer
    ssize_t r = read(fdin, buf+off, avail-off);
    if (r == 0) break;
    if (r == -1 && errno == EINTR) continue;
    cksys("read()", r);
    off += r;
  }

  // Truncate to final size
  safe_ftruncate(fdout, off);
  // munmap – no need; fexecve unmaps automatically
}

void transfer(int fdin, int fdout) {
  for (ssize_t r=1; r > 0; /* pass */) {
    // Transferring 2GB at a time; this should be portable for 32bit
    // systems (and linux complains at the max val for uint64_t)
    r = splice(fdin, NULL, fdout, NULL, ((size_t)1)<<31, 0);
    if (r < 0 && errno == EINTR) continue;
    if (r < 0 && errno == EINVAL) {
      // This might be a tty? Ttys are not supported by splice(2); fall back
      // to using a read()/mmap() based implementation
      // TODO: Figure out a way how to test this
      transfer_userspace(fdin, fdout);
      return;
    }
    cksys("splice()", r);
  }
}

int main(int argc __attribute__((unused)), char *argv[]) {
  // Try executing stdin in place; if this works, execution
  // of this program will terminate, so we can assume that some
  // error occurred if the program keeps going
  fexecve(0, argv, __environ);

  // OK; it's probably a file; copy into a anonymous, memory backed
  // temp file, then it should work
  const ssize_t f = cksys("memfd_create()", syscall(SYS_memfd_create, "Virtual File", MFD_CLOEXEC));
  transfer(0, f);

  cksys("fexecve()", fexecve(f, argv, __environ));
  fprintf(stderr, "Fatal Error in fexecve(): Should have terminated the process");
  return 1;
}
