#include <iostream>
#include <numeric> // for gcd
#include <algorithm> // for std::swap
#include <compare> // for <=> operator

// Fraction class to represent rational numbers
class Fraction {
private:
    long long numerator;
    long long denominator;

    // Helper function to simplify the fraction
    void simplify() {
        if (denominator == 0) {
            throw std::runtime_error("Denominator cannot be zero");
        }

        if (numerator == 0) {
            denominator = 1;
            return;
        }

        long long common_divisor = std::gcd(std::abs(numerator), std::abs(denominator));
        numerator /= common_divisor;
        denominator /= common_divisor;

        // Ensure denominator is always positive
        if (denominator < 0) {
            numerator *= -1;
            denominator *= -1;
        }
    }

public:
    // Default constructor (0/1)
    Fraction() : numerator(0), denominator(1) {}

    // Constructor with numerator and denominator
    Fraction(long long num, long long denom = 1) : numerator(num), denominator(denom) {
        simplify();
    }

    // Get numerator
    long long getNumerator() const { return numerator; }

    // Get denominator
    long long getDenominator() const { return denominator; }

    // Set numerator and denominator
    void set(long long num, long long denom) {
        numerator = num;
        denominator = denom;
        simplify();
    }

    // Get value as long double
    long double value() const {
        return static_cast<long double>(numerator) / denominator;
    }

    // Arithmetic operators

    Fraction operator+(const Fraction& other) const {
        long long new_num = numerator * other.denominator + other.numerator * denominator;
        long long new_den = denominator * other.denominator;
        return Fraction(new_num, new_den);
    }

    Fraction operator-(const Fraction& other) const {
        long long new_num = numerator * other.denominator - other.numerator * denominator;
        long long new_den = denominator * other.denominator;
        return Fraction(new_num, new_den);
    }

    Fraction operator*(const Fraction& other) const {
        long long new_num = numerator * other.numerator;
        long long new_den = denominator * other.denominator;
        return Fraction(new_num, new_den);
    }

    Fraction operator/(const Fraction& other) const {
        if (other.numerator == 0) {
            throw std::runtime_error("Division by zero");
        }
        long long new_num = numerator * other.denominator;
        long long new_den = denominator * other.numerator;
        return Fraction(new_num, new_den);
    }

    // Compound assignment operators
    Fraction& operator+=(const Fraction& other) {
        *this = *this + other;
        return *this;
    }

    Fraction& operator-=(const Fraction& other) {
        *this = *this - other;
        return *this;
    }

    Fraction& operator*=(const Fraction& other) {
        *this = *this * other;
        return *this;
    }

    Fraction& operator/=(const Fraction& other) {
        *this = *this / other;
        return *this;
    }

    // Comparison operator (C++20 spaceship operator)
    auto operator<=>(const Fraction& other) const {
        long long lhs = numerator * other.denominator;
        long long rhs = other.numerator * denominator;
        return lhs <=> rhs;
    }

    // Equality operator
    bool operator==(const Fraction& other) const {
        return numerator == other.numerator && denominator == other.denominator;
    }

    // Output operator
    friend std::ostream& operator<<(std::ostream& os, const Fraction& frac) {
        if (frac.denominator == 1) {
            os << frac.numerator;
        } else {
            os << frac.numerator << "/" << frac.denominator;
        }
        return os;
    }
};