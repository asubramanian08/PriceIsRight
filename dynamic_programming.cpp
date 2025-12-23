#include <cassert>
#include <iostream>
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
// NOTE: If you don't want to base your policy on knowing other player's, ignore policy_probability array

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

// Optimal 2nd player policy (for winning game) -- ASSUMING 2ND & 3RD PLAYER ACTS ACCORDING TO THEIR POLICY
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
            for (int spin = 0; spin <= 20; spin++) // player 3 spin (NOTE: 0 means they choose not to spin)
                for (int spinAgain = 0; spinAgain <= 1; spinAgain++) // spin again [1] or not [0]
                {
                    // Calculate the win probabilities for player 3 based on the decision
                    Fraction win_prob_player3;
                    if (spinAgain == 0) { // Don't spin again
                        if (spin > p1 && spin > p2) {
                            win_prob_player3 = Fraction(1, 1); // Wins outright
                        } else if (spin == p1 && spin == p2) {
                            win_prob_player3 = Fraction(1, 3); // Three-way tie (spinoff)
                        } else if (spin == max(p1, p2)) {
                            win_prob_player3 = Fraction(1, 2); // Two-way tie (spinoff)
                        } else {
                            win_prob_player3 = Fraction(0, 1); // Loses outright
                        }
                    } else { // Spin again
                        Fraction prob_new_spin(1, 20); // Uniform probability for each spin
                        for (int new_spin = 1; new_spin <= 20; new_spin++) {
                            // NOTE: If you spin 0 when spin again, its like spinning once
                            // NOTE: if you bust but everyone else busts, there is a one spin, spinoff
                            if (new_spin + spin > 20) { // bust (3rd might win if others busted)
                                win_prob_player3 += prob_new_spin * Fraction((p1 + p2 == 0 ? 1 : 0), 3);
                            } else if (new_spin + spin > p1 && new_spin + spin > p2) {
                                win_prob_player3 += prob_new_spin * Fraction(1, 1); // wins outright
                            } else if (new_spin + spin == p1 && new_spin + spin == p2) {
                                win_prob_player3 += prob_new_spin * Fraction(1, 3); // three-way tie
                            } else if (new_spin + spin == max(p1, p2)) { 
                                win_prob_player3 += prob_new_spin * Fraction(1, 2); // two-way tie
                            } else {
                                win_prob_player3 += prob_new_spin * Fraction(0, 1); // loses outright
                            }
                        }
                    }

                    // Store the calculated probability
                    third_player_probability[p1][p2][spin][spinAgain][2] = win_prob_player3;

                    // Calculate and store probabilities for players 1 and 2 similarly
                    if (p1 == p2)
                        third_player_probability[p1][p2][spin][spinAgain][0]
                            = third_player_probability[p1][p2][spin][spinAgain][1]
                            = Fraction(1, 2) * (Fraction(1, 1) - win_prob_player3);
                    else // p1 != p2
                    {
                        third_player_probability[p1][p2][spin][spinAgain][(p1 > p2 ? 0 : 1)] = Fraction(1, 1) - win_prob_player3; // winner
                        third_player_probability[p1][p2][spin][spinAgain][(p1 > p2 ? 1 : 0)] = Fraction(0, 1); // loser
                    }
                }

    // Calculate third person win rate based on their policy
    for (int p1 = 0; p1 <= 20; p1++) // player 1 total score
        for (int p2 = 0; p2 <= 20; p2++) // player 2 total score
        {
            // If policy wants to spin first spin
            if (third_player_policy(p1, p2, 0) == Fraction(1, 1)) {
                third_player_policy_probability[p1][p2][0] = third_player_probability[p1][p2][0][0][0]; // player 1 win
                third_player_policy_probability[p1][p2][1] = third_player_probability[p1][p2][0][0][1]; // player 2 win
                third_player_policy_probability[p1][p2][2] = third_player_probability[p1][p2][0][0][2]; // player 3 win
                continue; // skip to next
            }
            
            // Set all values to zero
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
            third_player_policy_probability[p1][p2][0] = player1_win;
            third_player_policy_probability[p1][p2][1] = player2_win;
            third_player_policy_probability[p1][p2][2] = player3_win;
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
        // If policy wants to spin first spin
        if (second_player_policy(p1, 0) == Fraction(1, 1)) {
            second_player_policy_probability[p1][0] = second_player_probability[p1][0][0][0]; // player 1 win
            second_player_policy_probability[p1][1] = second_player_probability[p1][0][0][1]; // player 2 win
            second_player_policy_probability[p1][2] = second_player_probability[p1][0][0][2]; // player 3 win
            continue; // skip to next
        }
        
        // Set all values to zero
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
        second_player_policy_probability[p1][0] = player1_win;
        second_player_policy_probability[p1][1] = player2_win;
        second_player_policy_probability[p1][2] = player3_win;
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
                for (int new_spin = 1; new_spin <= 20; new_spin++) {
                    int p1total = (spin1 + new_spin > 20) ? 0 : spin1 + new_spin;
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
    if (first_player_policy(0) == Fraction(1, 1)) { // If policy wants to spin first spin
        first_player_policy_probability[0] = first_player_probability[0][0][0]; // player 1 win
        first_player_policy_probability[1] = first_player_probability[0][0][1]; // player 2 win
        first_player_policy_probability[2] = first_player_probability[0][0][2]; // player 3 win
    }
    else { // If you do a first spin (normal)
        
        // Set all values to zero
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
        first_player_policy_probability[0] = player1_win;
        first_player_policy_probability[1] = player2_win;
        first_player_policy_probability[2] = player3_win;
    }
}

int main(void) {

    // -- Assumptions --
    // Uniform spin distribution from 1 to 20
    // There is an equal probability of anyone winning in the spinoff
    // Players plays according to their assigned policies: 
    auto third_player_policy = third_player_optimal_policy;
    auto second_player_policy = second_player_optimal_policy;
    auto first_player_policy = first_player_optimal_policy;

    
    // -- Run DP to fill tables --
    initialize_dp_tables(third_player_policy, second_player_policy, first_player_policy);

    // -- Output first player's win probability --
    std::cout << "First player's win probability: " << first_player_policy_probability[0] << std::endl
                << "Second player's win probability: " << first_player_policy_probability[1] << std::endl
                << "Third player's win probability: " << first_player_policy_probability[2] << std::endl;

    return 0;
}