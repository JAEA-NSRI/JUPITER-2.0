#include <stdio.h>
#include "list.h"

struct foo {
  struct geom_list list;
  int a;
};

int main(int argc, char **argv)
{
  int i;
  struct foo head;      // Using this variable as head.
  struct foo data[10];  // `geom_list` does not depend how
                        // base `struct` are allocated.
                        // So using array here.
  struct foo *fooptr;
  struct geom_list *listptr;

  for(i = 0; i < 10; ++i) {
    data[i].a = i;
    geom_list_init(&data[i].list); // init
  }
  geom_list_init(&head.list);      // init
  head.a = -1; // We define that the object which value of -1
               // for field a is the head; but head is fixed here,
               // definition of head is not required.

  // Add items in random order.
  // (inserting prev of `head` means append to the list)
  geom_list_insert_prev(&head.list, &data[5].list);
  geom_list_insert_prev(&head.list, &data[3].list);
  geom_list_insert_prev(&head.list, &data[2].list);
  geom_list_insert_prev(&head.list, &data[4].list);
  geom_list_insert_prev(&head.list, &data[8].list);
  geom_list_insert_prev(&head.list, &data[0].list);
  geom_list_insert_prev(&head.list, &data[9].list);
  geom_list_insert_prev(&head.list, &data[1].list);
  geom_list_insert_prev(&head.list, &data[7].list);
  geom_list_insert_prev(&head.list, &data[6].list);

  // Get pointer to head.
  listptr = &head.list;

  // Follow next items in 6 times.
  for (i = 0; i < 6; i++) {
    listptr = geom_list_next(listptr);
  }

  // Get pointer to `struct foo` from `listptr`
  fooptr = goem_list_entry(listptr, struct foo, list);

  printf("the sixth item is %d\n", fooptr->a); // prints '0'.

  // Goto next item.
  listptr = geom_list_next(listptr);

  // Remove `fooptr` from the list.
  geom_list_delete(&fooptr->list);

  // Goto previous item.
  listptr = geom_list_prev(listptr);
  fooptr = geom_list_entry(listptr, struct foo, list);

  printf("after remove: %d\n", fooptr->a); // prints '8'.

  // Iterate over the list
  i = 1;
  geom_list_foreach(listptr, &head.list) {
    fooptr = geom_list_entry(listptr, struct foo, list);
    printf("item at %d: %d\n", i, fooptr->a);
    // prints items in order of 5, 3, 2, 4, 8, 9, 1, 7 and 6.
    i++;
  }
  return 0;
}
