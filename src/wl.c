/* w32d.c */

#include "global.h"
#include "main2.h"

void OutOfMemory(void)
{
  puts("\n\nOut of memory!!!\n");
  abort();
}

int main(int argc, char **argv)
{
  main2(argc, argv);
  return (0);
}

