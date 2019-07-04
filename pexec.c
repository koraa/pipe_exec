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

extern char **environ;

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

void transfer_mmap(int fdin, int fdout) {
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

// Transfer data from one fd into the other using splice
// Returns 0 if the data was transferred successfully, -1
// if the underlying file type is not supported and exits
// on any other error.
int transfer_splice(int fdin, int fdout) {
  for (size_t cnt=0; true; cnt++) {
    // Transferring 2GB at a time; this should be portable for 32bit
    // systems (and linux complains at the max val for uint64_t)
    ssize_t r = splice(fdin, NULL, fdout, NULL, ((size_t)1)<<31, 0);
    if (r == 0) return 0; // We're done
    if (r < 0 && errno == EINTR) continue;
    if (r < 0 && errno == EINVAL && cnt == 0) return -1; // File not supported
    cksys("splice()", r);
  }
}

int main(int argc __attribute__((unused)), char *argv[]) {
  // Try executing stdin in place; if this works, execution
  // of this program will terminate, so we can assume that some
  // error occurred if the program keeps going
  fexecve(0, argv, environ);

  // OK; it's probably a file; copy into a anonymous, memory backed
  // temp file, then it should work
  const ssize_t f = cksys("memfd_create()", syscall(SYS_memfd_create, "Virtual File", MFD_CLOEXEC));
  if (transfer_splice(0, f) < 0)
    transfer_mmap(0, f);

  cksys("fexecve()", fexecve(f, argv, environ));
  fprintf(stderr, "Fatal Error in fexecve(): Should have terminated the process");
  return 1;
}
