#include <router.hpp>

#include <iostream>

int main()
{
    router r;
    r.add_route("/hello/<who>", [](std::string who) {
       std::cout << "hello " << who << std::endl;
    });
    r.add_route("/add/<left>/<right>", [](double left, double right) {
       std::cout << "add " << left << "+" << right << "=" << left + right << std::endl;
    });
    r("/hello/world");
    r("/add/21.2/20.8");
    return 0;
}
