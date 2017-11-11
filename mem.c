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
int memSize;
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
  memSize = size;
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
void printLL()
{
  printf("\n");
  struct Slot slot;
  void* slot_ptr = (struct Slot*)head_ptr;
  while(slot_ptr != NULL)
  {
    slot = *((struct Slot*)slot_ptr);
    printf("type: %d, size: %d, next:%d\n", slot.type, slot.size, slot.next);
    slot_ptr = slot.next;
  }
}
void* Mem_Alloc(int size)
{
  if(policy == MEM_POLICY_FIRSTFIT)
  {
    struct Slot slot;
    void* slot_ptr = (struct Slot*)head_ptr;
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
        return slot_ptr+sizeof(struct Slot)+1;
      }
      slot_ptr = slot.next;
    }
    return NULL;
  }
  else if(policy == MEM_POLICY_BESTFIT)
  {
    struct Slot slot;
    void* slot_ptr = (struct Slot*)head_ptr;
    void *bestFitSlot = NULL;
    int bestFit = memSize;
    while(slot_ptr != NULL)
    {
      slot = *((struct Slot*)slot_ptr);
      int fit = slot.size - size;
      if(slot.type == FREE && fit > 0 && fit < bestFit)
      {
        bestFitSlot = slot_ptr;
        bestFit = fit;
      }
      slot_ptr = slot.next;
    }
    if(bestFitSlot == NULL)
      return NULL;
    else {
      slot = *((struct Slot*)bestFitSlot);
      //split free memory
      if(slot.size > size)
      {
        //printf("splitting memory\n");
        struct Slot freeSlot;
        freeSlot.type = FREE;
        freeSlot.size = slot.size-size-sizeof(struct Slot);
        freeSlot.next = slot.next;
        void* freeSlotPtr = bestFitSlot+sizeof(struct Slot)+size;
        *((struct Slot*)freeSlotPtr) = freeSlot;
        slot.next = freeSlotPtr;
      }
      slot.size = size;
      slot.type = ALLC;
      *((struct Slot*)bestFitSlot) = slot;
      return bestFitSlot+sizeof(struct Slot)+1;
    }
  } 
  else if(policy == MEM_POLICY_WORSTFIT)
  {
    struct Slot slot;
    void* slot_ptr = (struct Slot*)head_ptr;
    void *worstFitSlot = NULL;
    int worstFit = 0;
    while(slot_ptr != NULL)
    {
      slot = *((struct Slot*)slot_ptr);
      int fit = slot.size - size;
      if(slot.type == FREE && fit > 0 && fit > worstFit)
      {
        worstFitSlot = slot_ptr;
        worstFit = fit;
      }
      slot_ptr = slot.next;
    }
    if(worstFitSlot == NULL)
      return NULL;
    else {
      slot = *((struct Slot*)worstFitSlot);
      //split free memory
      if(slot.size > size)
      {
        //printf("splitting memory\n");
        struct Slot freeSlot;
        freeSlot.type = FREE;
        freeSlot.size = slot.size-size-sizeof(struct Slot);
        freeSlot.next = slot.next;
        void* freeSlotPtr = worstFitSlot+sizeof(struct Slot)+size;
        *((struct Slot*)freeSlotPtr) = freeSlot;
        slot.next = freeSlotPtr;
      }
      slot.size = size;
      slot.type = ALLC;
      *((struct Slot*)worstFitSlot) = slot;
      return worstFitSlot+sizeof(struct Slot)+1;
    }
  }
}

int between(int start, int end, int val)
{
  return val >= start && val <= end;
}

int Mem_Free(void* ptr)
{
  if(ptr == NULL)
    return -1;
  struct Slot slot;
  void* prev_slot_ptr = NULL;
  void* slot_ptr = (struct Slot*)head_ptr;
  while(slot_ptr != NULL)
  {
    slot = *((struct Slot*)slot_ptr);
    void* start = slot_ptr+sizeof(struct Slot);
    void* end = start + slot.size;
    if(slot.type == ALLC && between(start, end, ptr))
    {
      int size = 0;
      struct Slot afterSlot;
      void* afterSlotPtr = slot.next;
      if(afterSlotPtr != NULL && ((struct Slot*)afterSlotPtr)->type == FREE)
      {
        afterSlot = *((struct Slot*)afterSlotPtr);
        size += sizeof(struct Slot) + ((struct Slot*)slot.next)->size;
        afterSlotPtr = afterSlot.next;
        //*((struct Slot*)afterSlot.next) = NULL;
      }
      if(prev_slot_ptr != NULL && ((struct Slot*)prev_slot_ptr)->type == FREE)
      {
        ((struct Slot*)prev_slot_ptr)->size += size + sizeof(struct Slot) + ((struct Slot*)slot_ptr)->size;
        ((struct Slot*)prev_slot_ptr)->next = afterSlotPtr;
        return 0;
      } else {
        ((struct Slot*)slot_ptr)->size += size;
        ((struct Slot*)slot_ptr)->type = FREE;
        ((struct Slot*)slot_ptr)->next = afterSlotPtr;
        return 0;
      }
      break;
    }
    prev_slot_ptr = slot_ptr;
    slot_ptr = slot.next;
  }
  return -1;
}

int Mem_IsValid(void* ptr)
{
  struct Slot slot;
  void* slot_ptr = (struct Slot*)head_ptr;
  while(slot_ptr != NULL)
  {
    slot = *((struct Slot*)slot_ptr);
    void* start = slot_ptr+sizeof(struct Slot);
    void* end = start + slot.size;
    if(slot.type == ALLC && between(start, end, ptr))
      return 1;
    slot_ptr = slot.next;
  }
  return 0;
}

float Mem_GetFragmentation()
{
  struct Slot slot;
  void* slot_ptr = (struct Slot*)head_ptr;
  float largest_slot = 0;
  float total_free = 0;
  while(slot_ptr != NULL)
  {
    slot = *((struct Slot*)slot_ptr);
    if(slot.type == FREE)
    {
      if(slot.size > largest_slot)
        largest_slot = slot.size;
      total_free += slot.size;
    }
    slot_ptr = slot.next;
  }
  if(largest_slot > 0 && total_free > 0)
    return largest_slot/total_free;
  else
    return 0;
}
