// Run Monte Carlo Simulations for Given Policies

#ifndef MONTE_CARLO_SIMULATION
#define MONTE_CARLO_SIMULATION

#include "fraction.h"
#include <vector>
#include <cstdlib>
#include <cassert>
#include <ctime>
#include <fstream>
using namespace std;

// --- Global Variables ---
// Simulation data: (1st spin 1) (1st spin 2) (2nd spin 1) (2nd spin 2) (3rd spin 1) (3rd spin 2) (player # - 1)
long long simulations[21][21][21][21][21][21][3]; // NOTE: First spin of a player is never 0


// -- Utility --
// probabilistic decision: return true with probability prob
bool random_decision(Fraction prob) {
    long rand_num = rand() % prob.getDenominator().get_si();
    return rand_num < prob.getNumerator().get_si();
}


// -- Simulate Game --
// simulation function that returns 3 fraction values Fraction[3]
vector<Fraction> simulate_game(
    Fraction (*third_player_policy)(int p1, int p2, int spin),
    Fraction (*second_player_policy)(int p1, int spin),
    Fraction (*first_player_policy)(int spin),
    long long num_simulations)
{
    // Assuming DP tables have been initialized

    // initialize score variables & dataset
    long long p1_wins = 0;
    long long p2_wins = 0;
    long long p3_wins = 0;
    srand(time(0)); // seed random number generator
    memset(simulations, 0, sizeof(simulations)); // set to 0

    // Loop many times
    for (long long sim = 0; sim < num_simulations; sim++) {
        
        // Player 1's turn
        int p1_s1 = rand() % 20 + 1, p1_s2 = 0, p1_total;
        Fraction policy1 = first_player_policy(p1_total = p1_s1); // Probability of spinning again
        if (p1_total == 20) // Not allowed to spin again on 20
            policy1 = Fraction(0, 1);
        if (random_decision(policy1)) { // spin again
            p1_s2 = rand() % 20 + 1;
            p1_total = p1_s1 + p1_s2;
            if (p1_total > 20) // bust
                p1_total = 0;
        }

        // Player 2's turn
        int p2_s1 = rand() % 20 + 1, p2_s2 = 0, p2_total;
        Fraction policy2 = second_player_policy(p1_total, p2_total = p2_s1); // Probability of spinning again
        if (p2_total == 20) // Not allowed to spin again on 20
            policy2 = Fraction(0, 1);
        if (random_decision(policy2)) { // spin again
            p2_s2 = rand() % 20 + 1;
            p2_total = p2_s1 + p2_s2;
            if (p2_total > 20) // bust
                p2_total = 0;
        }

        // Player 3's turn
        int p3_s1 = rand() % 20 + 1, p3_s2 = 0, p3_total;
        Fraction policy3 = third_player_policy(p1_total, p2_total, p3_total = p3_s1); // Probability of spinning again
        if (p3_total == 20) // Not allowed to spin again on 20
            policy3 = Fraction(0, 1);
        if (random_decision(policy3)) { // spin again
            p3_s2 = rand() % 20 + 1;
            p3_total = p3_s1 + p3_s2;
            if (p3_total > 20) // bust
                p3_total = 0;   
        }

        // Determine winner
        int max_score = max(p1_total, max(p2_total, p3_total));
        int num_winners = (p1_total == max_score) + (p2_total == max_score) + (p3_total == max_score);
        int selected_winner = rand() % num_winners; // select among winners uniformly (ASSUME SPIN OFF IS UNIFORM)
        if (p1_total == max_score) {
            if (selected_winner == 0) {
                p1_wins++;
                simulations[p1_s1][p1_s2][p2_s1][p2_s2][p3_s1][p3_s2][0]++;
                continue;
            }
            selected_winner--;
        }
        if (p2_total == max_score) {
            if (selected_winner == 0) {
                p2_wins++;
                simulations[p1_s1][p1_s2][p2_s1][p2_s2][p3_s1][p3_s2][1]++;
                continue;
            }
            selected_winner--;
        }
        if (p3_total == max_score) {
            p3_wins++;
            simulations[p1_s1][p1_s2][p2_s1][p2_s2][p3_s1][p3_s2][2]++;
            continue;
        }
    }

    // Return win probabilities
    assert(p1_wins + p2_wins + p3_wins == num_simulations);
    return {Fraction(p1_wins, num_simulations), Fraction(p2_wins, num_simulations), Fraction(p3_wins, num_simulations)};
}


// -- Export Data --
// Exports simulation data to CSV files for analysis
void export_simulation_data(string folder_path)
{
    // NOTE: Spin1 is never 0 (you must spin at least once)
    ofstream fs(folder_path + "/simulations.csv", ios::out);
    if (!fs.is_open()) {
        throw runtime_error("Failed to open file for writing simulation data");
    }
    fs << "P1Spin1,P1Spin2,P2Spin1,P2Spin2,P3Spin1,P3Spin2,P1Wins,P2Wins,P3Wins\n";
    for (int p1s1 = 1; p1s1 <= 20; p1s1++)
        for (int p1s2 = 0; p1s2 <= 20; p1s2++)
            for (int p2s1 = 1; p2s1 <= 20; p2s1++)
                for (int p2s2 = 0; p2s2 <= 20; p2s2++)
                    for (int p3s1 = 1; p3s1 <= 20; p3s1++)
                        for (int p3s2 = 0; p3s2 <= 20; p3s2++)
                            if (simulations[p1s1][p1s2][p2s1][p2s2][p3s1][p3s2][0] ||
                                simulations[p1s1][p1s2][p2s1][p2s2][p3s1][p3s2][1] ||
                                simulations[p1s1][p1s2][p2s1][p2s2][p3s1][p3s2][2])
                            {
                                fs << p1s1 << "," << p1s2 << "," << p2s1 << "," << p2s2 << "," << p3s1 << "," << p3s2 << ","
                                    << simulations[p1s1][p1s2][p2s1][p2s2][p3s1][p3s2][0] << ","
                                    << simulations[p1s1][p1s2][p2s1][p2s2][p3s1][p3s2][1] << ","
                                    << simulations[p1s1][p1s2][p2s1][p2s2][p3s1][p3s2][2] << "\n";
                            }
    fs.close();
}

#endif // MONTE_CARLO_SIMULATION