#include <iostream>

extern "C" {
void say_hello() { std::cout << "Hello, from cam_client_backend!\n"; }
}

int main(int argc, char **argv) {
    say_hello();
    return 0;
}
