#include <stdio.h>
#include <string>

void my_func(char *str) {
    fprintf(stdout, str);
}

void hi_not_hello() {
    fprintf(stdout, "Saying hi not hello\n");
}

void print_random_number() {
    int a = rand();
    fprintf(stdout, "Random number: %s\n", std::to_string(a).c_str());
}

void hello_teaser() {
    fprintf(stdout, "before the hello\n");
}

int main(int argc, char **argv) {
    print_random_number();
    hi_not_hello();
    hello_teaser();
    char *str = "Hello world!\n";
    my_func(str);
}