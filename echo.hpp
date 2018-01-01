#pragma once
#include <iostream>

template <typename T>
void echo(const char* expr, const T& value) {
    std::cout << expr << ": " << value << '\n';
}

void echo(const char* expr, const bool value) {
    std::cout << expr << ": " << (value?"true":"false") << '\n';
}

void echo(const char* expr, const long double value) {
    std::cout << expr << ": " << value << '\n';
}

void echo(const char* expr, const float value) {
    std::cout << expr << ": " << value << '\n';
}

void echo(const char* expr, const char value) {
    std::cout << expr << ": '" << value << "'\n";
}

void echo(const char* expr, const char* const value) {
    std::cout << expr << ": \"" << value << "\"\n";
}

void echo(const char* expr, const std::string& value) {
    std::cout << expr << ": \"" << value << "\"\n";
}

#define echo(expr) echo(#expr,(expr))
