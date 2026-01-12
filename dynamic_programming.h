// Dynamic Programming for Three-Player Spin Game
// Calculates win probabilities based on any given player policies

#ifndef DYNAMIC_PROGRAMMING_CPP
#define DYNAMIC_PROGRAMMING_CPP

#include <cassert>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cmath>
#include "fraction.h"
using namespace std;


// --- Arrays ---
// NOTE: The "spin" values can only be 1-20, 0 means the player chose not to do the first spin which isn't allowed
// Win probability: (1st player total) (2nd player total) (3rd player spin) (spin again [1] or not [0]) (player # - 1)
Fraction third_player_probability[21][21][21][2][3];
// Win probability: (1st player total) (2nd player total) (player # - 1)
Fraction third_player_policy_probability[21][21][3];  // incorporate third player policy
// Win probability: (1st player total) (2nd player spin) (spin again) (player # - 1)
Fraction second_player_probability[21][21][2][3];
// Win probability: (1st player total) (player # - 1)
Fraction second_player_policy_probability[21][3]; // incorporate second player policy
// Win probability: (1st player spin) (spin again) (player # - 1)
Fraction first_player_probability[21][2][3];
// Win probability: (player # - 1)
Fraction first_player_policy_probability[3]; // incorporate first player policy (expected win rates)
// Win probabilities of optimal play (with oracle knowledge):
Fraction optimal_third_player_probability[21][21][21][2][3];
Fraction optimal_third_player_policy_probability[21][21][3];
Fraction optimal_second_player_probability[21][21][2][3];
Fraction optimal_second_player_policy_probability[21][3];
Fraction optimal_first_player_probability[21][2][3];
Fraction optimal_first_player_policy_probability[3];


// ---- Policies -----
// NOTE: An ORACLE policy bases its decision on knowing later player's policies (violating the order of gameplay)
//       Most policies (non-oracle) act based on if later players act optimally (requires "optimal" DP tables filled)

// Optimal 3rd player policy (for winning game) ORACLE
Fraction third_player_oracle_optimal_policy(int player1_score, int player2_score, int spin1) {
    Fraction win_prob_if_spin = third_player_probability[player1_score][player2_score][spin1][1][2];
    Fraction win_prob_if_no_spin = third_player_probability[player1_score][player2_score][spin1][0][2];
    return (win_prob_if_spin > win_prob_if_no_spin) ? Fraction(1, 1) : Fraction(0, 1);
}
// Optimal 3rd player policy (for winning game) NOT ORACLE
Fraction third_player_optimal_policy(int player1_score, int player2_score, int spin1) {
    Fraction win_prob_if_spin = optimal_third_player_probability[player1_score][player2_score][spin1][1][2];
    Fraction win_prob_if_no_spin = optimal_third_player_probability[player1_score][player2_score][spin1][0][2];
    return (win_prob_if_spin > win_prob_if_no_spin) ? Fraction(1, 1) : Fraction(0, 1);
}

// Optimal 2nd player policy (for winning game) ORACLE -- ASSUMING IT KNOWS 3RD PLAYER POLICY
Fraction second_player_oracle_optimal_policy(int player1_score, int spin1) {
    Fraction win_prob_if_spin = second_player_probability[player1_score][spin1][1][1];
    Fraction win_prob_if_no_spin = second_player_probability[player1_score][spin1][0][1];
    return (win_prob_if_spin > win_prob_if_no_spin) ? Fraction(1, 1) : Fraction(0, 1);
}
// Optimal 2nd player policy (for winning game) NOT ORACLE
Fraction second_player_optimal_policy(int player1_score, int spin1) {
    Fraction win_prob_if_spin = optimal_second_player_probability[player1_score][spin1][1][1];
    Fraction win_prob_if_no_spin = optimal_second_player_probability[player1_score][spin1][0][1];
    return (win_prob_if_spin > win_prob_if_no_spin) ? Fraction(1, 1) : Fraction(0, 1);
}

// Optimal 1st player policy (for winning game) ORACLE -- ASSUMING IT KNOWS 2ND & 3RD PLAYER POLICY
Fraction first_player_oracle_optimal_policy(int spin1) {
    Fraction win_prob_if_spin = first_player_probability[spin1][1][0];
    Fraction win_prob_if_no_spin = first_player_probability[spin1][0][0];
    return (win_prob_if_spin > win_prob_if_no_spin) ? Fraction(1, 1) : Fraction(0, 1);
}
// Optimal 1st player policy (for winning game) NOT ORACLE
Fraction first_player_optimal_policy(int spin1) {
    Fraction win_prob_if_spin = optimal_first_player_probability[spin1][1][0];
    Fraction win_prob_if_no_spin = optimal_first_player_probability[spin1][0][0];
    return (win_prob_if_spin > win_prob_if_no_spin) ? Fraction(1, 1) : Fraction(0, 1);
}


// -- Initialize DP tables --
void initialize_dp_tables(Fraction (*third_player_policy)(int p1, int p2, int spin),
                          Fraction (*second_player_policy)(int p1, int spin),
                          Fraction (*first_player_policy)(int spin))
{
    // Calculate the 3rd player's options
    for (int p1 = 0; p1 <= 20; p1++) // player 1 total score
        for (int p2 = 0; p2 <= 20; p2++) // player 2 total score
            for (int spin1 = 0; spin1 <= 20; spin1++) // player 3 spin (NOTE: 0 means they choose not to spin)
                for (int spinAgain = 0; spinAgain <= 1; spinAgain++) // spin again [1] or not [0]
                {
                    // Skip invalid states 
                    if (spin1 == 0 || (spin1 == 20 && spinAgain)) // skiped first spin or spin again on 20
                    {
                        third_player_probability[p1][p2][spin1][spinAgain][0] = Fraction(-1, 1);
                        third_player_probability[p1][p2][spin1][spinAgain][1] = Fraction(-1, 1);
                        third_player_probability[p1][p2][spin1][spinAgain][2] = Fraction(-1, 1);
                        continue;
                    }
                    
                    // Initialize the win probabilities & other variables
                    Fraction player1_win(0, 1);
                    Fraction player2_win(0, 1);
                    Fraction player3_win(0, 1);
                    int max_score = max(p1, p2);
                    
                    // Calculate win probabilities
                    if (spinAgain == 0) { // Don't spin again
                        if (spin1 > max_score) {
                            player3_win = Fraction(1, 1); // wins outright (everything else 0)
                        } else if (spin1 < max_score) { // p3 looses
                            if (p1 == p2) // 2-way tie
                                player1_win = player2_win = Fraction(1, 2);
                            else // no tie
                                (p1 > p2 ? player1_win : player2_win) = Fraction(1, 1);
                        } else { // tie with winning player
                            if (p1 == p2) { // three-way tie
                                player1_win = player2_win = player3_win = Fraction(1, 3);
                            } else { // two-way tie
                                (p1 > p2 ? player1_win : player2_win) = Fraction(1, 2);
                                player3_win = Fraction(1, 2);
                            }
                        }
                    } else { // Spin again
                        Fraction prob_new_spin(1, 20); // Uniform probability for each spin
                        for (int spin2 = 1; spin2 <= 20; spin2++) {
                            // NOTE: If you spin 0 when spin again, its like spinning once
                            // NOTE: if you bust but everyone else busts, there is a spinoff of one spin
                            int total_score = (spin1 + spin2 > 20) ? 0 : spin1 + spin2;
                            if (total_score > max_score) {
                                player3_win += prob_new_spin * Fraction(1, 1); // wins outright (everything else 0)
                            } else if (total_score < max_score) { // p3 looses
                                if (p1 == p2) { // 2-way tie
                                    player1_win += prob_new_spin * Fraction(1, 2);
                                    player2_win += prob_new_spin * Fraction(1, 2);
                                } else // no tie
                                    (p1 > p2 ? player1_win : player2_win) += prob_new_spin * Fraction(1, 1);
                            } else { // tie with winning player
                                if (p1 == p2) { // three-way tie
                                    player1_win += prob_new_spin * Fraction(1, 3);
                                    player2_win += prob_new_spin * Fraction(1, 3);
                                    player3_win += prob_new_spin * Fraction(1, 3);
                                } else { // two-way tie
                                    (p1 > p2 ? player1_win : player2_win) += prob_new_spin * Fraction(1, 2);
                                    player3_win += prob_new_spin * Fraction(1, 2);
                                }
                            }
                        }
                    }
                    assert((player1_win + player2_win + player3_win) == Fraction(1, 1)); // Ensure probabilities sum to 1

                    // Store the calculated probability
                    third_player_probability[p1][p2][spin1][spinAgain][0] = player1_win;
                    third_player_probability[p1][p2][spin1][spinAgain][1] = player2_win;
                    third_player_probability[p1][p2][spin1][spinAgain][2] = player3_win;
                }

    // Calculate third person win rate based on their policy
    for (int p1 = 0; p1 <= 20; p1++) // player 1 total score
        for (int p2 = 0; p2 <= 20; p2++) // player 2 total score
        {
            // Set all values to zero (for non first spin part)
            Fraction player1_win(0, 1);
            Fraction player2_win(0, 1);
            Fraction player3_win(0, 1);

            // Run all spins (must do a first spin, spin!=0) for player 3
            Fraction spin_probability(1, 20); // uniform probability for each spin
            for (int spin = 1; spin <= 20; spin++)
            {
                Fraction policy = third_player_policy(p1, p2, spin); // probability of spinning again
                if (third_player_probability[p1][p2][spin][1][0] == Fraction(-1, 1)) // invalid state check
                    policy = Fraction(0, 1); // can't spin again if invalid state
                if (third_player_probability[p1][p2][spin][0][0] == Fraction(-1, 1)) // invalid state check
                    policy = Fraction(1, 1); // must spin again if invalid state
                player1_win += spin_probability * (policy * third_player_probability[p1][p2][spin][1][0]
                                    + (Fraction(1, 1) - policy) * third_player_probability[p1][p2][spin][0][0]);
                player2_win += spin_probability * (policy * third_player_probability[p1][p2][spin][1][1]
                                    + (Fraction(1, 1) - policy) * third_player_probability[p1][p2][spin][0][1]);
                player3_win += spin_probability * (policy * third_player_probability[p1][p2][spin][1][2]
                                    + (Fraction(1, 1) - policy) * third_player_probability[p1][p2][spin][0][2]);
            }
            assert((player1_win + player2_win + player3_win) == Fraction(1, 1)); // Ensure probabilities sum to 1

            // Set all array values
            third_player_policy_probability[p1][p2][0] = player1_win;
            third_player_policy_probability[p1][p2][1] = player2_win;
            third_player_policy_probability[p1][p2][2] = player3_win;
        }

    // Calculate the 2nd player's options
    for (int p1 = 0; p1 <= 20; p1++)
        for (int spin1 = 0; spin1 <= 20; spin1++) // player 2 spin (NOTE: 0 means they choose not to spin)
            for (int spinAgain = 0; spinAgain <= 1; spinAgain++) // spin again [1] or not [0]
            {
                // Skip invalid states 
                if (spin1 == 0 || (spin1 == 20 && spinAgain)) // skiped first spin or spin again on 20
                {
                    second_player_probability[p1][spin1][spinAgain][0] = Fraction(-1, 1);
                    second_player_probability[p1][spin1][spinAgain][1] = Fraction(-1, 1);
                    second_player_probability[p1][spin1][spinAgain][2] = Fraction(-1, 1);
                    continue;
                }
                
                // Calculate the win probabilities for player 2 based on the decision
                if (spinAgain == 0) { // Don't spin again
                    second_player_probability[p1][spin1][spinAgain][0] = third_player_policy_probability[p1][spin1][0];
                    second_player_probability[p1][spin1][spinAgain][1] = third_player_policy_probability[p1][spin1][1];
                    second_player_probability[p1][spin1][spinAgain][2] = third_player_policy_probability[p1][spin1][2];
                } else { // Spin again
                    // Initialize win probabilities
                    second_player_probability[p1][spin1][spinAgain][0] = Fraction(0, 1);
                    second_player_probability[p1][spin1][spinAgain][1] = Fraction(0, 1);
                    second_player_probability[p1][spin1][spinAgain][2] = Fraction(0, 1);
                    
                    // Run 20 possible new spins
                    Fraction prob_new_spin(1, 20); // Uniform probability for each spin
                    for (int new_spin = 1; new_spin <= 20; new_spin++) {
                        int p2total = (spin1 + new_spin > 20) ? 0 : spin1 + new_spin;
                        second_player_probability[p1][spin1][spinAgain][0] += prob_new_spin * third_player_policy_probability[p1][p2total][0];
                        second_player_probability[p1][spin1][spinAgain][1] += prob_new_spin * third_player_policy_probability[p1][p2total][1];
                        second_player_probability[p1][spin1][spinAgain][2] += prob_new_spin * third_player_policy_probability[p1][p2total][2];
                    }

                    assert((second_player_probability[p1][spin1][spinAgain][0]
                            + second_player_probability[p1][spin1][spinAgain][1]
                            + second_player_probability[p1][spin1][spinAgain][2]) == Fraction(1, 1)); // Ensure probabilities sum to 1
                }
            }
    
    // Calculate the second person win rate based on their policy
    for (int p1 = 0; p1 <= 20; p1++) // player 1 total score
    {
        // Set all values to zero (for non first spin part)
        Fraction player1_win(0, 1);
        Fraction player2_win(0, 1);
        Fraction player3_win(0, 1);

        // Run all spins (must do a first spin, spin!=0) for player 3
        Fraction spin_probability(1, 20); // uniform probability for each spin
        for (int spin = 1; spin <= 20; spin++)
        {
            Fraction policy = second_player_policy(p1, spin); // probability of spinning again
            if (second_player_probability[p1][spin][1][0] == Fraction(-1, 1)) // invalid state check
                policy = Fraction(0, 1); // can't spin again if invalid state
            if (second_player_probability[p1][spin][0][0] == Fraction(-1, 1)) // invalid state check
                policy = Fraction(1, 1); // must spin again if invalid state
            player1_win += spin_probability * (policy * second_player_probability[p1][spin][1][0]
                                + (Fraction(1, 1) - policy) * second_player_probability[p1][spin][0][0]);
            player2_win += spin_probability * (policy * second_player_probability[p1][spin][1][1]
                                + (Fraction(1, 1) - policy) * second_player_probability[p1][spin][0][1]);
            player3_win += spin_probability * (policy * second_player_probability[p1][spin][1][2]
                                + (Fraction(1, 1) - policy) * second_player_probability[p1][spin][0][2]);
        }
        assert((player1_win + player2_win + player3_win) == Fraction(1, 1)); // Ensure probabilities sum to 1

        // Set all array values
        second_player_policy_probability[p1][0] = player1_win;
        second_player_policy_probability[p1][1] = player2_win;
        second_player_policy_probability[p1][2] = player3_win;
    }

    // Calculate the 1st player's options
    for (int spin1 = 0; spin1 <= 20; spin1++) // player 1 spin (NOTE: 0 means they choose not to spin)
        for (int spinAgain = 0; spinAgain <= 1; spinAgain++) // spin again [1] or not [0]
        {
            // Skip invalid states 
            if (spin1 == 0 || (spin1 == 20 && spinAgain)) // skiped first spin or spin again on 20
            {
                first_player_probability[spin1][spinAgain][0] = Fraction(-1, 1);
                first_player_probability[spin1][spinAgain][1] = Fraction(-1, 1);
                first_player_probability[spin1][spinAgain][2] = Fraction(-1, 1);
                continue;
            }
            
            // Calculate the win probabilities for player 1 based on the decision
            if (spinAgain == 0) { // Don't spin again
                first_player_probability[spin1][spinAgain][0] = second_player_policy_probability[spin1][0];
                first_player_probability[spin1][spinAgain][1] = second_player_policy_probability[spin1][1];
                first_player_probability[spin1][spinAgain][2] = second_player_policy_probability[spin1][2];
            } else { // Spin again
                // Initialize win probabilities
                first_player_probability[spin1][spinAgain][0] = Fraction(0, 1);
                first_player_probability[spin1][spinAgain][1] = Fraction(0, 1);
                first_player_probability[spin1][spinAgain][2] = Fraction(0, 1);
                
                // Run 20 possible new spins
                Fraction prob_new_spin(1, 20); // Uniform probability for each spin
                for (int spin2 = 1; spin2 <= 20; spin2++) {
                    int p1total = (spin1 + spin2 > 20) ? 0 : spin1 + spin2;
                    first_player_probability[spin1][spinAgain][0] += prob_new_spin * second_player_policy_probability[p1total][0];
                    first_player_probability[spin1][spinAgain][1] += prob_new_spin * second_player_policy_probability[p1total][1];
                    first_player_probability[spin1][spinAgain][2] += prob_new_spin * second_player_policy_probability[p1total][2];
                }

                assert((first_player_probability[spin1][spinAgain][0]
                        + first_player_probability[spin1][spinAgain][1]
                        + first_player_probability[spin1][spinAgain][2]) == Fraction(1, 1)); // Ensure probabilities sum to 1
            }
        }

    // Calculate the first person win rate based on their policy
    {
        // Set all values to zero (for non first spin part)
        Fraction player1_win(0, 1);
        Fraction player2_win(0, 1);
        Fraction player3_win(0, 1);

        // Run all spins (must do a first spin, spin!=0) for player 3
        Fraction spin_probability(1, 20); // uniform probability for each spin
        for (int spin = 1; spin <= 20; spin++)
        {
            Fraction policy = first_player_policy(spin); // probability of spinning again
            if (first_player_probability[spin][1][0] == Fraction(-1, 1)) // invalid state check
                policy = Fraction(0, 1); // can't spin again if invalid state
            if (first_player_probability[spin][0][0] == Fraction(-1, 1)) // invalid state check
                policy = Fraction(1, 1); // must spin again if invalid state
            player1_win += spin_probability * (policy * first_player_probability[spin][1][0]
                                + (Fraction(1, 1) - policy) * first_player_probability[spin][0][0]);
            player2_win += spin_probability * (policy * first_player_probability[spin][1][1]
                                + (Fraction(1, 1) - policy) * first_player_probability[spin][0][1]);
            player3_win += spin_probability * (policy * first_player_probability[spin][1][2]
                                + (Fraction(1, 1) - policy) * first_player_probability[spin][0][2]);
        }
        assert((player1_win + player2_win + player3_win) == Fraction(1, 1)); // Ensure probabilities sum to 1

        // Set all array values
        first_player_policy_probability[0] = player1_win;
        first_player_policy_probability[1] = player2_win;
        first_player_policy_probability[2] = player3_win;
    }
}


// -- Export Data --
// Exports DP data to CSV files for analysis
void export_dp_data(string folder_path, auto policy3, auto policy2, auto policy1) {
    // NOTE: Assuming spin values can never be 0. 0 means you bust.

    // Player 3 DP data
    ofstream f3(folder_path + "/player3DP.csv", ios::out);
    if (!f3.is_open()) {
        throw runtime_error("Failed to open file for writing player 3 DP data");
    }
    f3 << "Player1Score,Player2Score,Player3Spin,WinProb1NoSpin,WinProb2NoSpin,WinProb3NoSpin,WinProb1Spin,WinProb2Spin,WinProb3Spin,PolicyDecision\n";
    for (int p1 = 0; p1 <= 20; p1++)
        for (int p2 = 0; p2 <= 20; p2++)
            for (int spin1 = 1; spin1 <= 20; spin1++)
            {
                f3 << p1 << "," << p2 << "," << spin1 << ","
                    << third_player_probability[p1][p2][spin1][0][0].value() << ","
                    << third_player_probability[p1][p2][spin1][0][1].value() << ","
                    << third_player_probability[p1][p2][spin1][0][2].value() << ","
                    << third_player_probability[p1][p2][spin1][1][0].value() << ","
                    << third_player_probability[p1][p2][spin1][1][1].value() << ","
                    << third_player_probability[p1][p2][spin1][1][2].value() << ","
                    << policy3(p1, p2, spin1).value() << "\n";
            }
    f3.close();

    // Player 2 DP data
    ofstream f2(folder_path + "/player2DP.csv", ios::out);
    if (!f2.is_open()) {
        throw runtime_error("Failed to open file for writing player 2 DP data");
    }
    f2 << "Player1Score,Player2Spin,WinProb1NoSpin,WinProb2NoSpin,WinProb3NoSpin,WinProb1Spin,WinProb2Spin,WinProb3Spin,PolicyDecision\n";
    for (int p1 = 0; p1 <= 20; p1++)
            for (int spin1 = 1; spin1 <= 20; spin1++)
            {
                f2 << p1 << "," << spin1 << ","
                    << second_player_probability[p1][spin1][0][0].value() << ","
                    << second_player_probability[p1][spin1][0][1].value() << ","
                    << second_player_probability[p1][spin1][0][2].value() << ","
                    << second_player_probability[p1][spin1][1][0].value() << ","
                    << second_player_probability[p1][spin1][1][1].value() << ","
                    << second_player_probability[p1][spin1][1][2].value() << ","
                    << policy2(p1, spin1).value() << "\n";
            }
    f2.close();

    // Player 1 DP data
    ofstream f1(folder_path + "/player1DP.csv", ios::out);
    if (!f1.is_open()) {
        throw runtime_error("Failed to open file for writing player 1 DP data");
    }
    f1 << "Player1Spin,WinProb1NoSpin,WinProb2NoSpin,WinProb3NoSpin,WinProb1Spin,WinProb2Spin,WinProb3Spin,PolicyDecision\n";
    for (int spin1 = 1; spin1 <= 20; spin1++)
            {
                f1 << spin1 << ","
                    << first_player_probability[spin1][0][0].value() << ","
                    << first_player_probability[spin1][0][1].value() << ","
                    << first_player_probability[spin1][0][2].value() << ","
                    << first_player_probability[spin1][1][0].value() << ","
                    << first_player_probability[spin1][1][1].value() << ","
                    << first_player_probability[spin1][1][2].value() << ","
                    << policy1(spin1).value() << "\n";
            }
    f1.close();
}

#endif // DYNAMIC_PROGRAMMING_CPP