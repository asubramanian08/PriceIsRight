// Analyze contestent using the quantal model of rationality

#ifndef CONTESTANT_RATIONALITY
#define CONTESTANT_RATIONALITY

#include <limits>
#include "fraction.h"
#include "dynamic_programming.h"
#include <vector>
using namespace std;


// --- Global Variables ---
// Quantal response parameter: how irrational to be in the policy
double lambda = 0;


// --- Utility ---
// Convert long double to Fraction using continued fractions
Fraction fromDouble(long double x, long long maxDen = 1000) {
    if (isnan(x) || isinf(x)) {
        throw runtime_error("Cannot convert NaN or Inf to Fraction");
    }
    // Handle sign
    bool neg = x < 0;
    x = abs(x);
    long long h0 = 0, h1 = 1;
    long long k0 = 1, k1 = 0;
    long double frac = x;
    while (true) {
        long long a = static_cast<long long>(floor(frac));
        long long h2 = a * h1 + h0;
        long long k2 = a * k1 + k0;
        if (k2 > maxDen) break;
        h0 = h1; h1 = h2;
        k0 = k1; k1 = k2;
        double remainder = frac - a;
        if (remainder < 1e-15) break;
        frac = 1.0 / remainder;
    }
    long long num = neg ? -h1 : h1;
    long long den = k1;
    return Fraction(num, den);
}


// --- Policies -----
// NOTE: the spin probabilities are approximated with fractions but its actually irrational
// Optimal 3rd player quantal policy (for winning game) NOT ORACLE
Fraction third_player_quantal_policy(int player1_score, int player2_score, int spin1) {
    Fraction win_prob_if_spin = optimal_third_player_probability[player1_score][player2_score][spin1][1][2];
    Fraction win_prob_if_no_spin = optimal_third_player_probability[player1_score][player2_score][spin1][0][2];
    long double deltaU = win_prob_if_spin.value() - win_prob_if_no_spin.value();
    long double spin_prob = 1.0 / (1.0 + exp(-lambda * deltaU));
    return fromDouble(spin_prob);
}
// Optimal 2nd player quantal policy (for winning game) NOT ORACLE
Fraction second_player_quantal_policy(int player1_score, int spin1) {
    Fraction win_prob_if_spin = optimal_second_player_probability[player1_score][spin1][1][1];
    Fraction win_prob_if_no_spin = optimal_second_player_probability[player1_score][spin1][0][1];
    long double deltaU = win_prob_if_spin.value() - win_prob_if_no_spin.value();
    long double spin_prob = 1.0 / (1.0 + exp(-lambda * deltaU));
    return fromDouble(spin_prob);
}
// Optimal 1st player quantal policy (for winning game) NOT ORACLE
Fraction first_player_quantal_policy(int spin1) {
    Fraction win_prob_if_spin = optimal_first_player_probability[spin1][1][0];
    Fraction win_prob_if_no_spin = optimal_first_player_probability[spin1][0][0];
    long double deltaU = win_prob_if_spin.value() - win_prob_if_no_spin.value();
    long double spin_prob = 1.0 / (1.0 + exp(-lambda * deltaU));
    return fromDouble(spin_prob);
}


// -- Test Rationality values --
// Run various lambda (rationality) values and output win rates for each contestant
void run_quantal_response(double end_lambda, int num_points,
                         vector<pair<double,array<long double, 3>>>& results,
                         auto third_player_policy,
                         auto second_player_policy,
                         auto first_player_policy) {
    // Generate lambda values logarithmically spaced
    vector<double> lambdas;
    double log_max = log(end_lambda + 1);
    for (int i = 0; i < num_points; ++i) {
        double t = static_cast<double>(i) / (num_points - 1);
        double l = exp(t * log_max) - 1;
        lambdas.push_back(l);
    }

    // Run for each lambda value
    results.clear();
    for (int i = 0; i < lambdas.size(); i++) {
        lambda = lambdas[i];
        initialize_dp_tables(third_player_policy, second_player_policy, first_player_policy);
        results.emplace_back(
            lambda,
            array<long double, 3>{
                first_player_policy_probability[0].value(),
                first_player_policy_probability[1].value(),
                first_player_policy_probability[2].value()
            }
        );
    }
}


// -- Export Data --
// Export win rates for various lambda (rationality) values to CSV
void export_quantal_response_data(string folder_path,
                                  vector<pair<double,array<long double, 3>>>& results) {
    ofstream fq(folder_path + "/quantal_response_results.csv", ios::out);
    if (!fq.is_open()) {
        throw runtime_error("Failed to open file for writing quantal response data");
    }
    // Note: this doesn't say anything about which players were tests as irrational or not
    fq << "Lambda,FirstPlayerWinProb,SecondPlayerWinProb,ThirdPlayerWinProb\n";
    for (const auto& result : results) {
        fq << result.first << "," << result.second[0] << "," << result.second[1] << "," << result.second[2] << "\n";
    }
    fq.close();
}

#endif // CONTESTANT_RATIONALITY