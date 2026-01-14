// Driver for collecting data and running simulations

#include <iostream>
#include "fraction.h"
#include "dynamic_programming.h"
#include "monte_carlo_simulation.h"
#include "contestant_rationality.h"
#include <vector>
#include <string>
using namespace std;


// -- Main Function --
int main(void) {

    // -- Assumptions --
    // Uniform spin distribution from 1 to 20 (Using 1-20 instead of 5-100 or 5 cents - $1)
    // There is an equal probability of anyone winning in the spinoff
    // No one is allowed to skip their first spin (including third player when both others bust)
    // You are not allowed to spin again if you get 20 in your first spin


    // -- Initialize "optimal" DP tables for reference --
    initialize_dp_tables(third_player_oracle_optimal_policy, second_player_oracle_optimal_policy, first_player_oracle_optimal_policy);
    copy(&third_player_probability[0][0][0][0][0], &third_player_probability[0][0][0][0][0] + sizeof(third_player_probability)/sizeof(Fraction), &optimal_third_player_probability[0][0][0][0][0]);
    copy(&third_player_policy_probability[0][0][0], &third_player_policy_probability[0][0][0] + sizeof(third_player_policy_probability)/sizeof(Fraction), &optimal_third_player_policy_probability[0][0][0]);
    copy(&second_player_probability[0][0][0][0], &second_player_probability[0][0][0][0] + sizeof(second_player_probability)/sizeof(Fraction), &optimal_second_player_probability[0][0][0][0]);
    copy(&second_player_policy_probability[0][0], &second_player_policy_probability[0][0] + sizeof(second_player_policy_probability)/sizeof(Fraction), &optimal_second_player_policy_probability[0][0]);
    copy(&first_player_probability[0][0][0], &first_player_probability[0][0][0] + sizeof(first_player_probability)/sizeof(Fraction), &optimal_first_player_probability[0][0][0]);
    copy(&first_player_policy_probability[0], &first_player_policy_probability[0] + sizeof(first_player_policy_probability)/sizeof(Fraction), &optimal_first_player_policy_probability[0]);


    // -- Run optimal policy & save data --
    // Run DP to fill tables
    initialize_dp_tables(third_player_optimal_policy, second_player_optimal_policy, first_player_optimal_policy);
    // Output win probabilities
    std::cout << "First player's win probability: " << first_player_policy_probability[0] << " (" << first_player_policy_probability[0].value() << ")" << std::endl
                << "Second player's win probability: " << first_player_policy_probability[1] << " (" << first_player_policy_probability[1].value() << ")" << std::endl
                << "Third player's win probability: " << first_player_policy_probability[2] << " (" << first_player_policy_probability[2].value() << ")" << std::endl << std::endl;
    // Run simulation based on policies
    vector<Fraction> simulated_win_rates = simulate_game(third_player_optimal_policy, second_player_optimal_policy, first_player_optimal_policy, 10'000'000);
    std::cout << "First player simulated wins: " << simulated_win_rates[0] << " (" << simulated_win_rates[0].value() << ")" << std::endl
                << "Second player simulated wins: " << simulated_win_rates[1] << " (" << simulated_win_rates[1].value() << ")" << std::endl
                << "Third player simulated wins: " << simulated_win_rates[2] << " (" << simulated_win_rates[2].value() << ")" << std::endl << std::endl;
    // Export data for analysis
    export_dp_data("./data/optimal_policy", third_player_optimal_policy, second_player_optimal_policy, first_player_optimal_policy);
    export_simulation_data("./data/optimal_policy");


    // // -- Run quantal response analysis & save data --
    // // Note this takes about a minute to run
    // vector<pair<double,array<long double, 3>>> quantal_win_probabilities;
    // // All 3 players use quantal policies
    // run_quantal_response(1000, 50, quantal_win_probabilities,
    //                      third_player_quantal_policy,
    //                      second_player_quantal_policy,
    //                      first_player_quantal_policy);
    // export_quantal_response_data("./data/quantal_response/all_quantal", quantal_win_probabilities);
    // // Test C3 quantal policy
    // run_quantal_response(1000, 50, quantal_win_probabilities,
    //                      third_player_quantal_policy,
    //                      second_player_optimal_policy,
    //                      first_player_optimal_policy);
    // export_quantal_response_data("./data/quantal_response/C3_quantal", quantal_win_probabilities);
    // // Test C2 quantal policy
    // run_quantal_response(1000, 50, quantal_win_probabilities,
    //                      third_player_optimal_policy,
    //                      second_player_quantal_policy,
    //                      first_player_optimal_policy);
    // export_quantal_response_data("./data/quantal_response/C2_quantal", quantal_win_probabilities);
    // // Test C1 quantal policy
    // run_quantal_response(1000, 50, quantal_win_probabilities,
    //                      third_player_optimal_policy,
    //                      second_player_optimal_policy,
    //                      first_player_quantal_policy);
    // export_quantal_response_data("./data/quantal_response/C1_quantal", quantal_win_probabilities);
    // cout << "Finished running irrational behavior analysis for C1, C2, C3, and all combined" << endl;

    
    // -- Parse the empirical data as CSV --


    return 0;
}