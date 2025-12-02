Solving the optimal strategies for the price is right wheel game.

Rules of the game:
* Three contestants that spin in sequence
* On your turn:
    1. Spin the wheel (with numbers 5, 10, 15, ..., 100)
    2. Choose to spin the wheel again. If the sum of the number is above 100 you lose. Otherwise your final score is the sum of the numbers you spun.
* The winner is the contestant who scored the highest
* If there is a tie for the highest, the contestants who scored that number do the whole game again to see who will win. The order of spinning remains the same

Good reference: https://fac.comtech.depaul.edu/rtenorio/Wheel.pdf

This is a solution for when each player plays optimally and each player knows that the other players are also playing optimally.

NEXT: use simulations to prove that the optimal strategy matches and that the probabilities also match
NEXT: put everything in camel_case and format comments correctly

Note: This ignores the thing where if you score 100 you spin again
Note: this assumes that if the 3rd player already won off of the 1st spin, they have to option to spin again and potentially lose or tie
 (this assumption can be changed if nessasary)