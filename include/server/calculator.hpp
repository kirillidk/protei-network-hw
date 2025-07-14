#pragma once
#include <string>

class Calculator {
public:
    static long long evaluate(const std::string& expression);
private:
    static long long parseExpression(const std::string& expr, size_t& pos);
    static long long parseTerm(const std::string& expr, size_t& pos);
    static long long parseFactor(const std::string& expr, size_t& pos);
    static long long parseNumber(const std::string& expr, size_t& pos);
};