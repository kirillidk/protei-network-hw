#include "server/calculator.hpp"
#include <stdexcept>
#include <cctype>

long long Calculator::evaluate(const std::string& expression) {
    size_t pos = 0;
    long long result = parseExpression(expression, pos);

    if (pos < expression.length()) {
        throw std::runtime_error("Unexpected character in expression");
    }

    return result;
}

long long Calculator::parseExpression(const std::string& expr, size_t& pos) {
    long long result = parseTerm(expr, pos);

    while (pos < expr.length()) {
        char op = expr[pos];
        if (op == '+' || op == '-') {
            ++pos;
            long long right = parseTerm(expr, pos);
            if (op == '+') {
                result += right;
            } else {
                result -= right;
            }
        } else {
            break;
        }
    }

    return result;
}

long long Calculator::parseTerm(const std::string& expr, size_t& pos) {
    long long result = parseFactor(expr, pos);

    while (pos < expr.length()) {
        char op = expr[pos];
        if (op == '*' || op == '/') {
            ++pos;
            long long right = parseFactor(expr, pos);
            if (op == '*') {
                result *= right;
            } else {
                if (right == 0) {
                    throw std::runtime_error("Division by zero");
                }
                result /= right;
            }
        } else {
            break;
        }
    }

    return result;
}

long long Calculator::parseFactor(const std::string& expr, size_t& pos) {
    if (pos >= expr.length()) {
        throw std::runtime_error("Unexpected end of expression");
    }

    if (expr[pos] == '(') {
        ++pos;
        long long result = parseExpression(expr, pos);
        if (pos >= expr.length() || expr[pos] != ')') {
            throw std::runtime_error("Missing closing parenthesis");
        }
        ++pos;
        return result;
    }

    if (expr[pos] == '-' || expr[pos] == '+') {
        bool negative = (expr[pos] == '-');
        ++pos;
        long long result = parseFactor(expr, pos);
        return negative ? -result : result;
    }

    return parseNumber(expr, pos);
}

long long Calculator::parseNumber(const std::string& expr, size_t& pos) {
    if (pos >= expr.length() || !std::isdigit(expr[pos])) {
        throw std::runtime_error("Expected number");
    }

    long long result = 0;

    while (pos < expr.length() && std::isdigit(expr[pos])) {
        result = result * 10 + (expr[pos] - '0');
        ++pos;
    }

    return result;
}