#include <valarray>
#include <vector>
#include <stdexcept>

// Assuming Fraction class exists with standard arithmetic operations
class Fraction {
    // Your Fraction implementation here
};

class Probability {
private:
    std::valarray<Fraction> probability; // size 4: [base, coeff1, coeff2, coeff3]
    std::vector<Probability> additionalAssumptions;
    
    // Helper for comparisons
    static bool isLessOrEqual(const Probability& lhs, const Probability& rhs, 
                             Fraction& a1_min, Fraction& a1_max,
                             Fraction& a2_min, Fraction& a2_max,
                             Fraction& a3_min, Fraction& a3_max,
                             std::vector<Probability>& assumptions);

public:
    // Constructors
    Probability() : probability(4) {} // all zeros
    Probability(const Fraction& base) : probability(4) { probability[0] = base; }
    Probability(const Fraction& base, const Fraction& coeff1, 
                const Fraction& coeff2, const Fraction& coeff3) 
        : probability{base, coeff1, coeff2, coeff3} {}
    Probability(const std::valarray<Fraction>& prob) : probability(prob) {
        if (prob.size() != 4) throw std::invalid_argument("Probability must have size 4");
    }
    Probability(const Probability&) = default;
    
    // Equality operators
    bool operator==(const Probability& other) const {
        return (probability == other.probability).min();
    }
    bool operator!=(const Probability& other) const {
        return !(*this == other);
    }
    
    // Arithmetic operators
    Probability operator+(const Probability& other) const {
        return Probability(probability + other.probability);
    }
    
    Probability operator-(const Probability& other) const {
        return Probability(probability - other.probability);
    }
    
    Probability& operator+=(const Probability& other) {
        probability += other.probability;
        return *this;
    }
    
    Probability& operator-=(const Probability& other) {
        probability -= other.probability;
        return *this;
    }
    
    Probability operator*(const Fraction& scalar) const {
        return Probability(probability * scalar);
    }
    
    Probability operator/(const Fraction& scalar) const {
        return Probability(probability / scalar);
    }
    
    Probability& operator*=(const Fraction& scalar) {
        probability *= scalar;
        return *this;
    }
    
    Probability& operator/=(const Fraction& scalar) {
        probability /= scalar;
        return *this;
    }
    
    // Comparison operators
    bool operator<(const Probability& other) const {
        Fraction a1_min, a1_max, a2_min, a2_max, a3_min, a3_max;
        std::vector<Probability> assumptions;
        return isLessOrEqual(*this, other, a1_min, a1_max, a2_min, a2_max, a3_min, a3_max, assumptions);
    }
    
    bool operator<=(const Probability& other) const {
        Fraction a1_min, a1_max, a2_min, a2_max, a3_min, a3_max;
        std::vector<Probability> assumptions;
        return isLessOrEqual(*this, other, a1_min, a1_max, a2_min, a2_max, a3_min, a3_max, assumptions);
    }
    
    bool operator>(const Probability& other) const {
        return !(*this <= other);
    }
    
    bool operator>=(const Probability& other) const {
        return !(*this < other);
    }
    
    // Getters
    const Fraction& base() const { return probability[0]; }
    const Fraction& coeff1() const { return probability[1]; }
    const Fraction& coeff2() const { return probability[2]; }
    const Fraction& coeff3() const { return probability[3]; }
    const std::vector<Probability>& getAdditionalAssumptions() const { return additionalAssumptions; }
};

// Helper function implementation
bool Probability::isLessOrEqual(const Probability& lhs, const Probability& rhs, 
                               Fraction& a1_min, Fraction& a1_max,
                               Fraction& a2_min, Fraction& a2_max,
                               Fraction& a3_min, Fraction& a3_max,
                               std::vector<Probability>& assumptions) {
    Probability diff = lhs - rhs;
    
    // Count non-zero coefficients
    int nonZeroCoeffs = 0;
    if (diff.coeff1() != Fraction(0)) nonZeroCoeffs++;
    if (diff.coeff2() != Fraction(0)) nonZeroCoeffs++;
    if (diff.coeff3() != Fraction(0)) nonZeroCoeffs++;
    
    if (nonZeroCoeffs == 0) {
        // Simple comparison with no variables
        return diff.base() <= Fraction(0);
    }
    else if (nonZeroCoeffs == 1) {
        // Single variable case - update thresholds
        if (diff.coeff1() != Fraction(0)) {
            // a1 is the only variable
            Fraction threshold = (-diff.base()) / diff.coeff1();
            if (diff.coeff1() > Fraction(0)) {
                // a1 <= threshold
                a1_max = threshold;
            } else {
                // a1 >= threshold
                a1_min = threshold;
            }
            return true;
        }
        else if (diff.coeff2() != Fraction(0)) {
            // a2 is the only variable
            Fraction threshold = (-diff.base()) / diff.coeff2();
            if (diff.coeff2() > Fraction(0)) {
                // a2 <= threshold
                a2_max = threshold;
            } else {
                // a2 >= threshold
                a2_min = threshold;
            }
            return true;
        }
        else if (diff.coeff3() != Fraction(0)) {
            // a3 is the only variable
            Fraction threshold = (-diff.base()) / diff.coeff3();
            if (diff.coeff3() > Fraction(0)) {
                // a3 <= threshold
                a3_max = threshold;
            } else {
                // a3 >= threshold
                a3_min = threshold;
            }
            return true;
        }
    }
    
    // Multiple variables case - add to assumptions
    assumptions.push_back(diff);
    return true;
}