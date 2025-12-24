#include <cassert>
#include <iostream>
#include <vector>
#include "fraction.cpp"
using namespace std;


// --- Arrays ---
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


// ---- Policies -----
// NOTE: If spin1 = 0, then return 1 if you wanted to not even do a first spin and 0 if you'll skip first spin
// PROBLEM: We give C1 and C2 data in their policies that describe exactly what later contestants policies are
//      something they don't have access to in a contest
//      If you really want to simulate without this knowledge, don't use the DP values in the policy

// Optimal 3rd player policy (for winning game): Spin again if less than max score
Fraction third_player_optimal_policy(int player1_score, int player2_score, int spin1) {
    if (spin1 == 0) {
        return Fraction(0, 1); // Always do a first spin
    }
    Fraction win_prob_if_spin = third_player_probability[player1_score][player2_score][spin1][1][2];
    Fraction win_prob_if_no_spin = third_player_probability[player1_score][player2_score][spin1][0][2];
    return (win_prob_if_spin > win_prob_if_no_spin) ? Fraction(1, 1) : Fraction(0, 1);
}

// Optimal 2nd player policy (for winning game) -- ASSUMING 3RD PLAYER ACTS ACCORDING TO THEIR POLICY
Fraction second_player_optimal_policy(int player1_score, int spin1) {
    if (spin1 == 0) {
        return Fraction(0, 1); // Always do a first spin
    }
    Fraction win_prob_if_spin = second_player_probability[player1_score][spin1][1][1];
    Fraction win_prob_if_no_spin = second_player_probability[player1_score][spin1][0][1];
    return (win_prob_if_spin > win_prob_if_no_spin) ? Fraction(1, 1) : Fraction(0, 1);
}

// Optimal 1st player policy (for winning game) -- ASSUMING 2ND & 3RD PLAYER ACTS ACCORDING TO THEIR POLICY
Fraction first_player_optimal_policy(int spin1) {
    if (spin1 == 0) {
        return Fraction(0, 1); // Always do a first spin
    }
    Fraction win_prob_if_spin = first_player_probability[spin1][1][1];
    Fraction win_prob_if_no_spin = first_player_probability[spin1][0][1];
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
                    // Initialize the win probabilities & other variables
                    Fraction player1_win(0, 1);
                    Fraction player2_win(0, 1);
                    Fraction player3_win(0, 1);
                    int max_score = max(p1, p2);
                    int winning_player = (p1 >= p2) ? 0 : 1; // 0 for player 1, 1 for player 2
                    
                    // Calculate win probabilities
                    if (spinAgain == 0) { // Don't spin again
                        if (spin1 > max_score) {
                            player3_win = Fraction(1, 1); // wins outright (everything else 0)
                        } else if (spin1 < max_score) {
                            (winning_player == 0 ? player1_win : player2_win) = Fraction(1, 1); // p3 looses
                        } else { // tie with winning player
                            if (p1 == p2) { // three-way tie
                                player1_win = player2_win = player3_win = Fraction(1, 3);
                            } else { // two-way tie
                                (winning_player == 0 ? player1_win : player2_win) = Fraction(1, 2);
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
                            } else if (total_score < max_score) {
                                (winning_player == 0 ? player1_win : player2_win) += prob_new_spin * Fraction(1, 1); // p3 looses
                            } else { // tie with winning player
                                if (p1 == p2) { // three-way tie
                                    player1_win += prob_new_spin * Fraction(1, 3);
                                    player2_win += prob_new_spin * Fraction(1, 3);
                                    player3_win += prob_new_spin * Fraction(1, 3);
                                } else { // two-way tie
                                    (winning_player == 0 ? player1_win : player2_win) += prob_new_spin * Fraction(1, 2);
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
            // If policy wants to skip first spin
            Fraction prob_of_first_spin = third_player_policy(p1, p2, 0);
            third_player_policy_probability[p1][p2][0] = prob_of_first_spin * third_player_probability[p1][p2][0][0][0]; // player 1 win
            third_player_policy_probability[p1][p2][1] = prob_of_first_spin * third_player_probability[p1][p2][0][0][1]; // player 2 win
            third_player_policy_probability[p1][p2][2] = prob_of_first_spin * third_player_probability[p1][p2][0][0][2]; // player 3 win
            
            // Set all values to zero (for non first spin part)
            Fraction player1_win(0, 1);
            Fraction player2_win(0, 1);
            Fraction player3_win(0, 1);

            // Run all spins (must do a first spin, spin!=0) for player 3
            Fraction spin_probability(1, 20); // uniform probability for each spin
            for (int spin = 1; spin <= 20; spin++)
            {
                Fraction policy = third_player_policy(p1, p2, spin); // probability of spinning again
                player1_win += spin_probability * (policy * third_player_probability[p1][p2][spin][1][0]
                                    + (Fraction(1, 1) - policy) * third_player_probability[p1][p2][spin][0][0]);
                player2_win += spin_probability * (policy * third_player_probability[p1][p2][spin][1][1]
                                    + (Fraction(1, 1) - policy) * third_player_probability[p1][p2][spin][0][1]);
                player3_win += spin_probability * (policy * third_player_probability[p1][p2][spin][1][2]
                                    + (Fraction(1, 1) - policy) * third_player_probability[p1][p2][spin][0][2]);
            }
            assert((player1_win + player2_win + player3_win) == Fraction(1, 1)); // Ensure probabilities sum to 1

            // Set all array values
            third_player_policy_probability[p1][p2][0] += (Fraction(1, 1) - prob_of_first_spin) * player1_win;
            third_player_policy_probability[p1][p2][1] += (Fraction(1, 1) - prob_of_first_spin) * player2_win;
            third_player_policy_probability[p1][p2][2] += (Fraction(1, 1) - prob_of_first_spin) * player3_win;
        }

    // Calculate the 2nd player's options
    for (int p1 = 0; p1 <= 20; p1++)
        for (int spin1 = 0; spin1 <= 20; spin1++) // player 2 spin (NOTE: 0 means they choose not to spin)
            for (int spinAgain = 0; spinAgain <= 1; spinAgain++) // spin again [1] or not [0]
            {
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
        // If policy wants to skip first spin
        Fraction prob_of_first_spin = second_player_policy(p1, 0);
        second_player_policy_probability[p1][0] = prob_of_first_spin * second_player_probability[p1][0][0][0]; // player 1 win
        second_player_policy_probability[p1][1] = prob_of_first_spin * second_player_probability[p1][0][0][1]; // player 2 win
        second_player_policy_probability[p1][2] = prob_of_first_spin * second_player_probability[p1][0][0][2]; // player 3 win
        
        // Set all values to zero (for non first spin part)
        Fraction player1_win(0, 1);
        Fraction player2_win(0, 1);
        Fraction player3_win(0, 1);

        // Run all spins (must do a first spin, spin!=0) for player 3
        Fraction spin_probability(1, 20); // uniform probability for each spin
        for (int spin = 1; spin <= 20; spin++)
        {
            Fraction policy = second_player_policy(p1, spin); // probability of spinning again
            player1_win += spin_probability * (policy * second_player_probability[p1][spin][1][0]
                                + (Fraction(1, 1) - policy) * second_player_probability[p1][spin][0][0]);
            player2_win += spin_probability * (policy * second_player_probability[p1][spin][1][1]
                                + (Fraction(1, 1) - policy) * second_player_probability[p1][spin][0][1]);
            player3_win += spin_probability * (policy * second_player_probability[p1][spin][1][2]
                                + (Fraction(1, 1) - policy) * second_player_probability[p1][spin][0][2]);
        }
        assert((player1_win + player2_win + player3_win) == Fraction(1, 1)); // Ensure probabilities sum to 1

        // Set all array values
        second_player_policy_probability[p1][0] += (Fraction(1, 1) - prob_of_first_spin) * player1_win;
        second_player_policy_probability[p1][1] += (Fraction(1, 1) - prob_of_first_spin) * player2_win;
        second_player_policy_probability[p1][2] += (Fraction(1, 1) - prob_of_first_spin) * player3_win;
    }

    // Calculate the 1st player's options
    for (int spin1 = 0; spin1 <= 20; spin1++) // player 1 spin (NOTE: 0 means they choose not to spin)
        for (int spinAgain = 0; spinAgain <= 1; spinAgain++) // spin again [1] or not [0]
        {
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
        // If the policy wants to skip first spin
        Fraction prob_of_first_spin = first_player_policy(0);
        first_player_policy_probability[0] = prob_of_first_spin * first_player_probability[0][0][0]; // player 1 win
        first_player_policy_probability[1] = prob_of_first_spin * first_player_probability[0][0][1]; // player 2 win
        first_player_policy_probability[2] = prob_of_first_spin * first_player_probability[0][0][2]; // player 3 win
        
        // Set all values to zero (for non first spin part)
        Fraction player1_win(0, 1);
        Fraction player2_win(0, 1);
        Fraction player3_win(0, 1);

        // Run all spins (must do a first spin, spin!=0) for player 3
        Fraction spin_probability(1, 20); // uniform probability for each spin
        for (int spin = 1; spin <= 20; spin++)
        {
            Fraction policy = first_player_policy(spin); // probability of spinning again
            player1_win += spin_probability * (policy * first_player_probability[spin][1][0]
                                + (Fraction(1, 1) - policy) * first_player_probability[spin][0][0]);
            player2_win += spin_probability * (policy * first_player_probability[spin][1][1]
                                + (Fraction(1, 1) - policy) * first_player_probability[spin][0][1]);
            player3_win += spin_probability * (policy * first_player_probability[spin][1][2]
                                + (Fraction(1, 1) - policy) * first_player_probability[spin][0][2]);
        }
        assert((player1_win + player2_win + player3_win) == Fraction(1, 1)); // Ensure probabilities sum to 1

        // Set all array values
        first_player_policy_probability[0] += (Fraction(1, 1) - prob_of_first_spin) * player1_win;
        first_player_policy_probability[1] += (Fraction(1, 1) - prob_of_first_spin) * player2_win;
        first_player_policy_probability[2] += (Fraction(1, 1) - prob_of_first_spin) * player3_win;
    }
}


// -- Simulate Game --
// probabilistic decision: return true with probability prob
bool random_decision(Fraction prob) {
    long long rand_num = rand() % prob.getDenominator();
    return rand_num < prob.getNumerator();
}
// simulation function that returns 3 fraction values Fraction[3]
vector<Fraction> simulate_game(
    Fraction (*third_player_policy)(int p1, int p2, int spin),
    Fraction (*second_player_policy)(int p1, int spin),
    Fraction (*first_player_policy)(int spin),
    long long num_simulations)
{
    // Assuming DP tables have been initialized

    // initialize score variables
    long long p1_wins = 0;
    long long p2_wins = 0;
    long long p3_wins = 0;
    srand(time(0)); // seed random number generator

    // Loop many times
    for (long long sim = 0; sim < num_simulations; sim++) {
        
        // Player 1's turn
        int p1_total = 0;
        if (!random_decision(first_player_policy(0))) { // Don't skip first spin
            int p1_total = rand() % 20 + 1; // first spin
            Fraction policy1 = first_player_policy(p1_total); // Probability of spinning again
            if (random_decision(policy1)) { // spin again
                p1_total += rand() % 20 + 1;
                if (p1_total > 20) // bust
                    p1_total = 0;
            }
        }

        // Player 2's turn
        int p2_total = 0;
        if (!random_decision(second_player_policy(p1_total, 0))) { // Don't skip first spin
            int p2_total = rand() % 20 + 1; // first spin
            Fraction policy2 = second_player_policy(p1_total, p2_total); // Probability of spinning again
            if (random_decision(policy2)) { // spin again
                p2_total += rand() % 20 + 1;
                if (p2_total > 20) // bust
                    p2_total = 0;
            }
        }

        // Player 3's turn
        int p3_total = 0;
        if (!random_decision(third_player_policy(p1_total, p2_total, 0))) { // Don't skip first spin
            int p3_total = rand() % 20 + 1; // first spin
            Fraction policy3 = third_player_policy(p1_total, p2_total, p3_total); // Probability of spinning again
            if (random_decision(policy3)) { // spin again
                p3_total += rand() % 20 + 1;
                if (p3_total > 20) // bust
                    p3_total = 0;   
            }
        }

        // Determine winner
        int max_score = max(p1_total, max(p2_total, p3_total));
        int num_winners = (p1_total == max_score) + (p2_total == max_score) + (p3_total == max_score);
        int selected_winner = rand() % num_winners; // select among winners uniformly (ASSUME SPIN OFF IS UNIFORM)
        if (p1_total == max_score) {
            if (selected_winner == 0) {
                p1_wins++;
                continue;
            }
            selected_winner--;
        }
        if (p2_total == max_score) {
            if (selected_winner == 0) {
                p2_wins++;
                continue;
            }
            selected_winner--;
        }
        if (p3_total == max_score) {
            p3_wins++;
            continue;
        }
    }

    // Return win probabilities
    return {Fraction(p1_wins, num_simulations), Fraction(p2_wins, num_simulations), Fraction(p3_wins, num_simulations)};
}



int main(void) {

    // -- Assumptions --
    // Uniform spin distribution from 1 to 20
    // There is an equal probability of anyone winning in the spinoff
    // No one is allowed to skip their first spin (including third player when both others bust)
    // You are not allowed to spin again if you get 20 in your first spin
    // Players plays according to their assigned policies: 
    auto third_player_policy = third_player_optimal_policy;
    auto second_player_policy = second_player_optimal_policy;
    auto first_player_policy = first_player_optimal_policy;

    
    // -- Run DP to fill tables --
    initialize_dp_tables(third_player_policy, second_player_policy, first_player_policy);
    // -- Output win probabilities --
    std::cout << "First player's win probability: " << first_player_policy_probability[0] << std::endl
                << "Second player's win probability: " << first_player_policy_probability[1] << std::endl
                << "Third player's win probability: " << first_player_policy_probability[2] << std::endl << std::endl;


    // -- Run simulation based on policies --
    vector<Fraction> simulated_win_rates = simulate_game(third_player_policy, second_player_policy, first_player_policy, 1'000'000);
    std::cout << "First player simulated wins: " << simulated_win_rates[0] << std::endl
                << "Second player simulated wins: " << simulated_win_rates[1] << std::endl
                << "Third player simulated wins: " << simulated_win_rates[2] << std::endl << std::endl;

    return 0;
}