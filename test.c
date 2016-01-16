#include <stdio.h>
#include <stdlib.h>

int *helper(int size) {
   printf("Helper\n");
   int* v = malloc(size * sizeof(int));
   if(!v) printf("Malloc (helper) failed\n");
   int* v2 = calloc(size, sizeof(int));
   v2[1] = 3;
   free(v2);
   return v;   
}

int* do_mem(int size) {
    int i;
    for(i = 0; i < size; i++) {
      int* x = helper(size);   
      x[0] = 3;
      x = realloc(x, (1024 + i) * sizeof(int));
      x[0] = 4;
      free(x);
    }
    int* y = malloc(size * sizeof(int));
    if(!y) printf("Malloc (do_mem) failed\n");
    return y;
}

int main() {
    int* a = do_mem(10);
    a[0] = 1;
    free(a);
    return 0;
}
