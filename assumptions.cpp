#include <iostream>
#include <algorithm>
#include "fraction.cpp"

// Note: there is a little fuzziness between (< and <=) and (> and >=)
class Assumption {
private:
    Fraction value;      // Current assumed value
    Fraction min_bound;  // Minimum possible value for which the assumption holds
    Fraction max_bound;  // Maximum possible value for which the assumption holds
    std::string name;    // Identifier for this assumption

public:
    // Constructor
    Assumption(Fraction initial_value, std::string var_name, 
               Fraction initial_min = Fraction(0, 100), 
               Fraction initial_max = Fraction(100, 100))
        : value(initial_value), 
          min_bound(initial_min),
          max_bound(initial_max),
          name(var_name) {}

    // Comparison operators that update bounds
    bool operator>(const Fraction& other) {
        if (value > other) {
            min_bound = std::max(min_bound, other);
            return true;
        } else {
            max_bound = std::min(max_bound, other);
            return false;
        }
    }

    bool operator<(const Fraction& other) {
        if (value < other) {
            max_bound = std::min(max_bound, other);
            return true;
        } else {
            min_bound = std::max(min_bound, other);
            return false;
        }
    }

    bool operator>=(const Fraction& other) { return !(*this < other); }
    bool operator<=(const Fraction& other) { return !(*this > other); }

    // Friend comparisons
    friend bool operator>(const Fraction& lhs, Assumption& rhs) { return rhs < lhs; }
    friend bool operator<(const Fraction& lhs, Assumption& rhs) { return rhs > lhs; }
    friend bool operator>=(const Fraction& lhs, Assumption& rhs) { return rhs <= lhs; }
    friend bool operator<=(const Fraction& lhs, Assumption& rhs) { return rhs >= lhs; }

    // Getters
    Fraction get_value() const { return value; }
    Fraction get_min() const { return min_bound; }
    Fraction get_max() const { return max_bound; }
    std::string get_name() const { return name; }

    // Check if current bounds are valid
    bool is_valid(Fraction true_value) const {
        return min_bound <= true_value && true_value <= max_bound;
    }

    // Print current status
    void print_status() const {
        std::cout << "Assumption " << name << ": " << value 
                  << " (range: [" << min_bound << ", " << max_bound << "])" << std::endl;
    }

    void print_status(Fraction true_value) const {
        std::cout << "Assumption " << name << ": " << value 
                  << " (range: [" << min_bound << ", " << max_bound << "])"
                  << "IS " << (is_valid(true_value) ? " VALID" : " INVALID")
                  << " true value = " << true_value << std::endl;
    }
};



