// Define a rational number class using GMP for arbitrary precision

#ifndef FRACTION
#define FRACTION

#include <iostream>
#include <numeric> // for gcd
#include <algorithm> // for std::swap
#include <compare> // for <=> operator
#include <gmpxx.h>
using namespace std;

// Fraction class to represent rational numbers
class Fraction {
private:
    mpz_class numerator;
    mpz_class denominator;

    // Helper function to simplify the fraction
    void simplify() {
        if (denominator == 0) {
            throw std::runtime_error("Denominator cannot be zero");
        }

        if (numerator == 0) {
            denominator = 1;
            return;
        }

        mpz_class common_divisor;
        mpz_gcd(common_divisor.get_mpz_t(), numerator.get_mpz_t(), denominator.get_mpz_t());
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
    Fraction(mpz_class num, mpz_class denom = 1) : numerator(num), denominator(denom) {
        simplify();
    }

    // Constructor with numerator and denominator
    Fraction(long num, long denom = 1) : numerator(num), denominator(denom) {
        simplify();
    }

    // Get numerator
    mpz_class getNumerator() const { return numerator; }

    // Get denominator
    mpz_class getDenominator() const { return denominator; }

    // Set numerator and denominator
    void set(mpz_class num, mpz_class denom) {
        numerator = num;
        denominator = denom;
        simplify();
    }

    // Get value as long double
    double value() const {
        // NOTE: This is a double, not long double
        return  numerator.get_d() / denominator.get_d();
    }

    // Arithmetic operators

    Fraction operator+(const Fraction& other) const {
        mpz_class new_num = numerator * other.denominator + other.numerator * denominator;
        mpz_class new_den = denominator * other.denominator;
        return Fraction(new_num, new_den);
    }

    Fraction operator-(const Fraction& other) const {
        mpz_class new_num = numerator * other.denominator - other.numerator * denominator;
        mpz_class new_den = denominator * other.denominator;
        return Fraction(new_num, new_den);
    }

    Fraction operator*(const Fraction& other) const {
        mpz_class new_num = numerator * other.numerator;
        mpz_class new_den = denominator * other.denominator;
        return Fraction(new_num, new_den);
    }

    Fraction operator/(const Fraction& other) const {
        if (other.numerator == 0) {
            throw std::runtime_error("Division by zero");
        }
        mpz_class new_num = numerator * other.denominator;
        mpz_class new_den = denominator * other.numerator;
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

    // Equality operator
    bool operator==(const Fraction& other) const {
        return numerator == other.numerator && denominator == other.denominator;
    }

    // Comparison operator (C++20 spaceship operator)
    auto operator<=>(const Fraction& other) const {
        mpz_class lhs = numerator * other.denominator;
        mpz_class rhs = other.numerator * denominator;
        if (lhs < rhs) return std::strong_ordering::less;
        if (lhs > rhs) return std::strong_ordering::greater;
        return std::strong_ordering::equal;
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

#endif // FRACTION
