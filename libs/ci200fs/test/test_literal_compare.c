/* test_literal_compare.c - Compare array vs mapping literal bytecode */
#include <sys.h>

create() {
  int *arr;
  mapping m;
  
  /* Array literal assignment */
  arr = ({ "one", "two", "three" });
  
  /* Mapping literal assignment */
  m = ([ "key": "value" ]);
}
