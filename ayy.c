#include <stdio.h>
#include "mem.c"

#define REGION_SIZE (10*1024)

void* myalloc(int size)
{
  printf("allocate memory of size=%d bytes...", size);
  void* p = Mem_Alloc(size);
  if(p) printf("  success (p=%p, f=%g)\n", p, Mem_GetFragmentation());
  else printf("  failed\n");
  return p;
}

int main(int argc, char* argv[])
{
  //myalloc(1000);

  printf("init memory allocator...");
  if(Mem_Init(REGION_SIZE, MEM_POLICY_FIRSTFIT) < 0) {
    printf("  unable to initialize memory allocator!\n");
    return -1;
  } else printf("  success!\n");

  printf("init memory allocator, again...");
  if(Mem_Init(REGION_SIZE, MEM_POLICY_FIRSTFIT) < 0)
    printf("  failed, but this is expected behavior!\n");
  else {
    printf("  success, which means the program incorrectly handles duplicate init...\n");
    return -1;
  }
  
  void* x1 = myalloc(64);
  void* p1 = myalloc(200);
  void* x2 = myalloc(64);
  void* p2 = myalloc(100);
  void* x3 = myalloc(64);
  void* p3 = myalloc(100000);
  void* x4 = myalloc(64);
  void* p4 = myalloc(500);
  void* x5 = myalloc(64);

}
