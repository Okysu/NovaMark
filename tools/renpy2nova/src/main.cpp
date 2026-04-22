#include "../include/renpy2nova/app/application.h"

#include <iostream>
int main(int argc, char** argv) {
    return nova::renpy2nova::run_application(argc, argv, std::cout, std::cerr);
}
