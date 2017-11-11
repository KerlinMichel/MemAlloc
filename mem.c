#include "mem.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>
#include <math.h>

int policy;
void* head_ptr;
int inited = 0;
#define FREE 0
#define ALLC 1

struct Slot{
    int type;
    int size;
    struct slot* next;
};

int Mem_Init(int size, int policy_)
{
  if(inited == 1)
    return -1;
  policy = policy_;
  //open the /dev/zero device
  int fd = open("/dev/zero", O_RDWR);
  // size (in bytes) must be divisible by page size
  double page_size = getpagesize();
  double pages = ((double)size)/page_size;
  size = (ceil(pages) * page_size);
  void *ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
  if (ptr == MAP_FAILED) {
    perror("mmap");
    return -1;
  }
  struct Slot head;
  head.type = FREE;
  head.size = size-sizeof(struct Slot);
  head.next = NULL;
  *(struct Slot*)ptr = head;
  head_ptr = ptr;
  //printf("%d\n", ptr);
  //printf("%d\n", ((struct Slot*)ptr)->size);
  //printf("%d\n", policy);
  // close the device (don't worry, mapping should be unaffected)
  close(fd);
  inited = 1;
  return 0;
}

void* Mem_Alloc(int size)
{
  if(policy == MEM_POLICY_FIRSTFIT)
  {
    struct Slot slot;
    void* slot_ptr = (struct Slot*)head_ptr;
    //slot = *((struct Slot*)head_ptr);
    while(slot_ptr != NULL)
    {
      slot = *((struct Slot*)slot_ptr);
      if(slot.size >= size && slot.type == FREE)
      {
        //split free memory
        if(slot.size > size)
        {
          //printf("splitting memory\n");
          struct Slot freeSlot;
          freeSlot.type = FREE;
          freeSlot.size = slot.size-size-sizeof(struct Slot);
          freeSlot.next = slot.next;
          void* freeSlotPtr = slot_ptr+sizeof(struct Slot)+size;
          *((struct Slot*)freeSlotPtr) = freeSlot;
          slot.next = freeSlotPtr;
        }
        slot.size = size;
        slot.type = ALLC;
        *((struct Slot*)slot_ptr) = slot;
        break;//return slot_ptr+sizeof(struct Slot)+1;
      }
      slot_ptr = slot.next;
      //slot = *((struct Slot*)slot_ptr);
      //printf("iter\n");
    }
    slot_ptr = (struct Slot*)head_ptr;
    while(slot_ptr != NULL)
    {
      slot = *((struct Slot*)slot_ptr);
      printf("\ntype: %d, size: %d, next:%d\n", slot.type, slot.size, slot.next);
      slot_ptr = slot.next;
    }
    return NULL;
    //printf("\nsize: %d\n", slot.size);
  }
}
