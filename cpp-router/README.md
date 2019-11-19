# CPP Router

> General purpose url router

## Example:

```cpp
#include <router.hpp>

int main(int argc, char** argv) {
    if (argc != 2) {
        return -1;
    }
    xdev::router r;
    r.add_route("/hello/<who = world>", [](std::string who) {
        std::cout << "hello " << who << std::endl;
    });
    r.add_route("/add/<left>/<right>", [](int left, int right) {
       std::cout << "add " << left << "+" << right << "=" << left + right << std::endl;
    });
    r(argv[1]);
    return 0;
}

```
