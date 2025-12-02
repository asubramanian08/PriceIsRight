#include <iostream>
#include <valarray>
#include <cassert>
#include "fraction.cpp"
using namespace std;

// Create a "probability" type
typedef valarray<Fraction> probability; // always size 4

int main()
{

    // SETUP

    // Notes about probability(valarray<Fraction>):
    // This represents an expression of size 4. The values of the expression is
    // va[0] + (size2_win2 * va[1]) + (size3_win3 * va[2]) + (size3_win2 * va[3]) where
    // size(i)_win(j) is the probability of winning with i participants as player number j.
    // Effectively, treat the "probability" as the probability as a certain player winning

    // Establish assumptions: (size2_win2, size3_win3, size3_win2)
    // Why these assumptions are nessasary: When a player is making the decision of weather to
    // spin again in a tie situation, they need to know if it is more beneficial to spin again
    // and risk busting or to do nothing with the chance to win in the battle of ties.
    // ie: if the score is player1=20 and player2=10 and player3 just spun a 20, it is best to
    // spin again since the probability of not busting and winning (16/20) is greater than the
    // probability of winning in a spin off of size 2 where they play second (size2_win2).
    cout << "ASSUMPTIONS" << endl;
    Fraction init_size2_win2(53, 100);
    Fraction init_size3_win3(35, 100);
    Fraction init_size3_win2(33, 100);
    cout << "Assumption: (size2_win2) The probability of winning as the 2nd person with 2 people in the wheel game is " << init_size2_win2 << endl;
    cout << "Assumption: (size3_win3) The probability of winning as the 3rd person with 3 people in the wheel game is " << init_size3_win3 << endl;
    cout << "Assumption: (size3_win2) The probability of winning as the 2nd person with 3 people in the wheel game is " << init_size3_win2 << endl;
    // Note: The assumption is valid even if it is not exact only if each decision made with
    // that assumption creates a threshold in which the actual number lies (since you
    /// wouldn't have made any decisions differently if you had the true number)
    // ie: in the situation where player1=x and player2=x and player3 just spun x, you should
    // only spin again if the chance of winning now (ie chance of not busting) > size3_win3.
    // therefore, this chance of not busting (which is (20-x)/20 ) is now a threshold on
    // size3_win3 since it was compared against size3_win3 in order to make a decision. if the
    // true value of size3_win3 fall in between the thresholds then all decisions were made
    // correctly and the assumption holds.
    // ie: I have assumed that size(i)_win(j)_min < true size(i)_win(j) < size(i)_win(j)_max
    // ^ that is the true assumption, not the single "init" number
    Fraction size2_win2_min(0, 100), size2_win2_max(100, 100);
    Fraction size3_win3_min(0, 100), size3_win3_max(100, 100);
    Fraction size3_win2_min(0, 100), size3_win2_max(100, 100);

    // General use
    Fraction zero(0, 1);
    Fraction one(1, 1);
    probability zeros(4);
    zeros = zero; // Initialize all probabilities to zero
    probability win = zeros, size2_win2 = zeros, size3_win3 = zeros, size3_win2 = zeros;
    win[0] = Fraction(1, 1);        // Set the first element to 1 (the main probability)
    size2_win2[1] = Fraction(1, 1); // 100% * probability of size2_win2
    size3_win3[2] = Fraction(1, 1); // 100% * probability of size3_win3
    size3_win2[3] = Fraction(1, 1); // 100% * probability of size3_win2

    // STEP 1: Calculate the probabilities of the third player

    // Set up the array
    probability thirdPerson[21][21][3];
    // (player 1 total score) (player 2 total score) (player #) = (probability of player # winning)
    // Note: a total score 0 means the player busted, so the player cannot win (that's why we have 21, not 20)

    // Calculate the probabilities for combination
    for (int p1 = 0; p1 < 21; p1++) // player 1 total score
    {
        for (int p2 = 0; p2 < 21; p2++) // player 2 total score
        {
            // Set up
            probability win3 = zeros; // player 3 winning probability
            int competition = max(p1, p2);
            Fraction chance3tiesThisRound = zero;
            Fraction chance3loosesThisRound = zero;

            // Case 1: spin1 > competition
            Fraction case1chance = Fraction(20 - competition, 20); // chance spin1 > competition
            probability case1win = Fraction(20, 20) * win; // always win if spin1 > competition
            win3 += case1chance * case1win;
            chance3tiesThisRound += case1chance * zero;
            chance3loosesThisRound += case1chance * zero;

            // Case 2: spin1 < competition
            // Note: if competition == 0, this case is impossible --- case2chance=0 and case2#win is wrong (but will not be used)
            Fraction case2chance = Fraction(max(competition - 1, 0), 20); // chance spin1 < competition
            // Case 2a - Chance of winning on spin2 (given spin1):
            // amount of busts = spin1
            // amount beating competition = 20 - (competition - spin1)  --> when (spin1+spin2) > competition
            // amount of wins = (amount beating competition) - (amount of busts) = 20 - competition
            probability case2awin = Fraction(20 - competition, 20) * win;
            // Case 2b - Chance of winning due to an inital tie and winning on the next round: 1/20 * size(?)_win(last)
            probability case2bwin;
            if (p1 != p2) // 2-way tie
                case2bwin = Fraction(1, 20) * size2_win2;
            else // 3-way tie (if competition != 0)
                case2bwin = Fraction(1, 20) * size3_win3;
            win3 += case2chance * (case2awin + case2bwin);
            chance3tiesThisRound += case2chance * Fraction(1, 20);
            // # looses = 20 - (# wins this round + # ties this round) = 20 - (20 - competition) - 1 = competition - 1
            chance3loosesThisRound += case2chance * Fraction(competition - 1, 20);

            // Case 3: spin1 == competition
            Fraction case3chance = Fraction(competition != 0 ? 1 : 0, 20); // chance spin1 == competition
            // Determining weather to spin again or not: (note - adjust for size3_win3 if 3-way tie)
            // probability of winning if I spin again: (20-competition)/20 --> chance of not busting
            // probability of winning if I don't spin again: size2_win2
            // spin again if: (20-competition)/20 > size2_win2
            // Using init_size2_win2 instead and updating the min and max values
            probability case3win; // either spin or not depending on what has the best chance of winning
            if (p1 != p2) // 2-way tie
            {
                if (Fraction(20 - competition, 20) > init_size2_win2) // Spin again
                {
                    case3win = Fraction(20 - competition, 20);
                    size2_win2_max = min(size2_win2_max, Fraction(20 - competition, 20)); // get a tighter bound
                    chance3tiesThisRound += case3chance * zero;
                    chance3loosesThisRound += case3chance * Fraction(competition, 20); // chance of a bust
                }
                else // Don't spin again
                {
                    case3win = size2_win2;
                    size2_win2_min = max(size2_win2_min, Fraction(20 - competition, 20)); // get a tighter bound
                    chance3tiesThisRound += case3chance * one;
                    chance3loosesThisRound += case3chance * zero;
                }
            }
            else // 3-way tie (replace size2_win2 with size3_win3)
            {
                if (Fraction(20 - competition, 20) > init_size3_win3) // Spin again
                {
                    case3win = Fraction(20 - competition, 20);
                    size3_win3_max = min(size3_win3_max, Fraction(20 - competition, 20)); // get a tighter bound
                    chance3tiesThisRound += case3chance * zero;
                    chance3loosesThisRound += case3chance * Fraction(competition, 20); // chance of a bust
                }
                else // Don't spin again
                {
                    case3win = size3_win3;
                    size3_win3_min = max(size3_win3_min, Fraction(20 - competition, 20)); // get a tighter bound
                    chance3tiesThisRound += case3chance * one;
                    chance3loosesThisRound += case3chance * zero;
                }
            }
            win3 += case3chance * case3win;

            // Update the "thirdPerson" array
            thirdPerson[p1][p2][2] = win3;
            if (p1 > p2) {
                thirdPerson[p1][p2][0] = win - win3;
                thirdPerson[p1][p2][1] = zeros;
            }
            else if (p1 < p2) {
                thirdPerson[p1][p2][1] = win - win3;
                thirdPerson[p1][p2][0] = zeros;
            }
            else // p1 == p2
            {
                thirdPerson[p1][p2][0] = chance3loosesThisRound * (win - size2_win2) // 3rd looses & 1st wins in the face off
                                         + chance3tiesThisRound * (win - size3_win3 - size3_win2); // 3rd ties & 1st wins in the face off
                thirdPerson[p1][p2][1] = chance3loosesThisRound * size2_win2 // 3rd looses & 2nd wins in the face off
                                         + chance3tiesThisRound * size3_win2; // 3rd ties & 1nd wins in the face off
                assert((thirdPerson[p1][p2][0] + thirdPerson[p1][p2][1] + win3 != win).sum() == 0); // Ensure the probabilities sum to 1
                if (p1 == 0 && p2 == 0) {
                    assert((thirdPerson[p1][p2][0] != zeros).sum() == 0); // Player 1 cannot win
                    assert((thirdPerson[p1][p2][1] != zeros).sum() == 0); // Player 2 cannot win
                }
            }
        }
    }

    // STEP 2: Calculate the probabilities of the second player

    // Set up the array
    

    return 0;
}