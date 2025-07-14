#include "server/calculator.hpp"

int main() {
    long long result = Calculator::evaluate("((10+5)*2-6)/4");  // 6

    return 0;
}