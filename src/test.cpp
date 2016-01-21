#include <iostream>

int *helper(int size) {
  std::cout << "Helper" << std::endl;
  int* v = NULL;
  try {
    v = new int[size];
  } catch(std::bad_alloc& ex) {
    std::cout << "OOM!" << std::endl;
  }
  if(!v)
    std::cout << "Malloc (helper) failed" << std::endl;
  return v;
}

int* do_mem(int size) {
  int i;
  for(i = 0; i < size; i++) {
    int* x = helper(size);
    x[0] = 3;
    delete[] x;
  }
  int* y = new int;
  if(!y)
    std::cout << "Malloc (do_mem) failed" << std::endl;
  return y;
}

int main() {
  int* a = do_mem(10);
  *a = 1;
  delete a;
  return 0;
}
