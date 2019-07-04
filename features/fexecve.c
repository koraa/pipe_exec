int main(void) {
  extern int fexecve(int, char *const[], char *const[]);
  char *cmd[] = { "dummy", (char *)0 };
  char *env[] = { "HOME=/", (char *)0 };
  fexecve(0, cmd, env);
  return 0;
}
