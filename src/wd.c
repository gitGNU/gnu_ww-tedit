/* w32d.c */

#include "global.h"
#include "main2.h"

#include <sys/exceptn.h>

void OutOfMemory(void)
{
  puts("\n\nOut of memory!!!\n");
  abort();
}

/* Minimize the size of the executable for GCC */

int __crt0_glob_function (void)
{
  return 0;
}

void __crt0_load_environment_file (void)
{
}

int main(int argc, char **argv)
{
  __djgpp_set_ctrl_c(0);  /* let ctrl+c to be an usual key combination */
  main2(argc, argv);
  return (0);
}

