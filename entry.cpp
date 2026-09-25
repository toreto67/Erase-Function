#include <cstdint>
#include <string>
#include <vector>
#include <windows.h>

#include "include/erase_funcs.h"

__declspec(noinline) void example_function() {
  printf("Hello World!\r\n");
  erase_end;
}

int main() {
  example_function();
  erase_fn(example_function);
  printf("[+] Function Erased!\r\n");
  std::cin.get();
}
