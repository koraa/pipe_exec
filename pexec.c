#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <linux/memfd.h>
#include <fcntl.h>

ssize_t cksys(const char *msg, ssize_t r) {
  if (r >= 0) return r;
  fprintf(stderr, "Fatal Error in %s: %s\n", msg, strerror(errno));
  _exit(1);
}

void transfer(int fdin, int fdout) {
  for (ssize_t r=1; r > 0; /* pass */) {
    // Transferring 2GB at a time; this should be portable for 32bit
    // systems (and linux complains at the max val for uint64_t)
    r = splice(fdin, NULL, fdout, NULL, ((size_t)1)<<31, 0);
    if (r < 0 && errno == EINTR) continue;
    if (r < 0 && errno == EINVAL) {
      fprintf(stderr, "Fatal Error in splice(): %s.\nStdin must be a file or pipe. Is it?\n", strerror(errno));
      _exit(1);
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
