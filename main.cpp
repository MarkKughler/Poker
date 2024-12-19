
#include <iostream>
#include <vector>
#include <string>
#include <random>           // default random engine
#include <chrono>           // time count
#include <io.h>             // console _setmode 
#include <fcntl.h>          // UTF16 no BOM
#include <bitset>
#define _ec(x) "\x1b["#x"m" // console color manipulator
// https://en.wikipedia.org/wiki/Glossary_of_poker_terms

#include "ascii_mover.h"

std::vector<int> deck_ids = 
{
 // A,  2,  3,  4,  5,  6,  7,  8,  9, 10,  J,  Q,  K        // 0=>12 modulo(13) value mapping  
        1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13,   // club grouping
       14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26,   // diamond grouping
       27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39,   // spade grouping
       40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52,   // heart grouping
       53, 54, 55                                            // three wildcard jokers
};
unsigned deal_index = 0;                                     // sequential iteration (todo: auto bounds wrapping??? ((n) % 52)
//std::default_random_engine rng;                              // random engine instance
const int max_players = 3;                                   // single deck game (shuffle beginning of each round)

const char* PokerHandName[31] =
{                                                                
    "high card", "", "", "",       // 0
    "one pair", "", "", "",        // 4        (2(2))
    "two pair", "", "", "",        // 8        (2(4))
    "three of a kind", "",         // 12       (2(6))
    "straight",                    // 14        
    "flush",                       // 15  
    "full house", "", "", "",      // 16       (2(8))
    "", "", "", "",                //     
    "four of a kind", "", "", "",  // 24       (2(12))
    "five of a kind",              // 28       (2(14))
    "staight flush",               // 29         
    "royal straight flush"         // 30        
};

struct HandInfo
{
    int cards[5];                 // actual card ID from the deck
    int high_card;                // used on zero ranking
    std::bitset<3> jokers;        // track jokers in hand
    int rank;                     // used for determining hand strength
    COORD pos;                    // this players screen position 
};
HandInfo dealer_hand  = { 0 };    // top center
HandInfo player1_hand = { 0 };    // right
HandInfo player2_hand = { 0 };    // bottom center
HandInfo player3_hand = { 0 };    // left



static void Deal()
{
    std::random_device rd;
    std::mt19937 mte(rd());
   
    deal_index = 0;
    for(int i=0; i<7; i++)
        std::shuffle(deck_ids.begin(), deck_ids.end(), mte);

    for (int i = 0; i < 5; ++i)
    {
        player2_hand.cards[i] = deck_ids.at(deal_index++); 
        dealer_hand.cards[i] = deck_ids.at(deal_index++);
    }

    player2_hand.high_card = 0;
    player2_hand.rank = 0;
    player2_hand.jokers.reset();
    dealer_hand.high_card = 0;
    dealer_hand.rank = 0;
    dealer_hand.jokers.reset();
}


static void RankHand(HandInfo* hand)
{
    //   solveable by comparison   |   determine by additional steps                    
    // ============================|=================================
    //  nothing       =  0  (0)    |          (high card)
    //  one pair      =  2  (4)    |
    //  two pair      =  4  (8)    |
    //  three of kind =  6  (12)   |
    //                      (14)   |          (straight)
    //                      (15)   |            (flush)                        
    //  full house    =  8  (16)   |
    //  four of kind  = 12  (24)   |
    //  five of kind  = 14  (28)   |                                     <---- at least one joker present
    //                      (29)   |         (straight flush)
    //                      (30)   |      (royal straight flush)

    hand->high_card = 0;
    hand->rank = 0;
    hand->jokers.reset();
    bool flush_found    = false;
    bool straight_found = false;
    bool make_ace_high  = false;
    

    // find high card ---------------------------------------------------------------------------
    for (int i = 0; i < 5; i++)
    {
        if (hand->cards[i] > 52)
        {   // handle jokers
            if (hand->cards[i] > hand->high_card)
                hand->high_card = hand->cards[i];
            if (hand->cards[i] == 53) hand->jokers.set(0);
            if (hand->cards[i] == 54) hand->jokers.set(1);
            if (hand->cards[i] == 55) hand->jokers.set(2);
        }
        else 
        {   // handle standard deck
            int a = hand->high_card % 13;
            int b = hand->cards[i] % 13;
            if (a == 0) a = 13;
            if (b == 0) b = 13;
            if (hand->high_card > 52)
            {
                if (hand->cards[i] > hand->high_card)
                    hand->high_card = hand->cards[i];
            }
            else if ((b > a))  hand->high_card = hand->cards[i];
            else if (b == a)
            {   // choose the higher suit of this card value
                if (hand->cards[i] > hand->high_card)
                    hand->high_card = hand->cards[i];
            }
        }
    }

    size_t offset = hand->jokers.count();
    // process rank solvable by comparison -----------------------------------------------------------
    for (int i = 0; i < 5; i++) 
    {
        for (int j = 0; j < 5; j++)
        {
            if (hand->cards[i] > 52 || hand->cards[j] > 52) continue;

            if (i != j && ((hand->cards[j] % 13) == (hand->cards[i] % 13)))
                hand->rank++;
        }
    }   
    hand->rank *= 2;                                                            // double result to make room for additional ranks
    if ((hand->rank > 0) && (offset == 0)) return;                              // early out (can not be straight or flush)
    // handle jokers
    if ((hand->rank == 12) && (offset == 1)) { hand->rank = 24; return; }       // three of a kind -> four of a kind
    if ((hand->rank == 12) && (offset == 2)) { hand->rank = 28; return; }       // three of a kind -> five of a kind
    if ((hand->rank == 8) && (offset == 1))  { hand->rank = 16; return; }       // two pair -> full house
    if ((hand->rank == 4) && (offset == 1))  { hand->rank = 12; return; }       // one pair -> three of a kind
    if ((hand->rank == 4) && (offset == 2))  { hand->rank = 24; return; }       // one pair -> four of a kind
    if ((hand->rank == 4) && (offset == 3))  { hand->rank = 28; return; }       // one pair -> five of a kind
   
    // continue evaluation checking for flush hand ---------------------------------------------------
    std::bitset<4> suit_bitset;
    for (int i = 0; i < 5; i++)
    {
        if (hand->cards[i] < 14) suit_bitset.set(0);                            // clubs
        if (hand->cards[i] > 13 && hand->cards[i] < 27) suit_bitset.set(1);     // diamonds
        if (hand->cards[i] > 26 && hand->cards[i] < 40) suit_bitset.set(2);     // spades
        if (hand->cards[i] > 39 && hand->cards[i] < 53) suit_bitset.set(3);     // hearts
    }                                                                           // absolutely no need to test for jokers present
    if (suit_bitset.count() == 1) flush_found = true;
    
    // check for straight ----------------------------------------------------------------------------
    std::sort(hand->cards, hand->cards + 5, 
        [](const int& first, const int& second) -> bool
        {   // keeping connection to deck representation (handle Ace later)
            if ((first < 53) && (second < 53)) 
            {   // don't check jokers here
                return ((first % 13) < (second % 13));
            }
            else {
                // handle joker
                return first > second; // stack jokers up front in decending order
            }
        }
    );
    straight_found = true; //default true for sequential test
    if ((hand->cards[4] % 13 == 12) && (hand->cards[offset] % 13 == 0))
    {   // King is present. swap Ace to the back.
        make_ace_high = true;
        std::rotate(&hand->cards[offset], &hand->cards[offset] + 1, &hand->cards[5]);
    }
    int error_count = (int)offset+1;
    for (size_t i = offset; i < 4; i++)
    {
        int a = hand->cards[i] % 13;
        int b = hand->cards[i + 1] % 13;
        if (make_ace_high)
        {
            if (a == 0) a = 13;
            if (b == 0) b = 13;
        }

        if(b - a != 1)
        {
            if (b - a <= error_count)
            {
                error_count -= (b - a);
                continue;
            }
            else
                straight_found = false;
        }
    }

    // final rank determination from gathered information --------------------------------------------
    if (straight_found && flush_found)                                                
    {
        if ((hand->cards[4] % 13 == 0))  hand->rank = 30;                             // royal flush
        else if ((hand->cards[4] % 13 == 12) && (offset == 1)) hand->rank = 30;       //     |
        else if ((hand->cards[4] % 13 == 11) && (offset == 2)) hand->rank = 30;       //     |
        else if ((hand->cards[4] % 13 == 10) && (offset == 3)) hand->rank = 30;       //     V
        else hand->rank = 29;                                                         // straight flush
    }
    else if (flush_found) hand->rank = 15;                                            // flush
    else if (straight_found) hand->rank = 14;                                         // straight
    else if (offset == 1) hand->rank = 4;                                             // (joker) one pair
    else if (offset == 2) hand->rank = 12;                                            // (jokers) three of kind
    else if (offset == 3) hand->rank = 24;                                            // (jokers) four of kind
}


void Display(int card)
{   // set color
    if (card < 14) std::cout << _ec(34);
    if (card > 13 && card < 27) std::cout << _ec(31);
    if (card > 26 && card < 40) std::cout << _ec(34);
    if (card > 39 && card < 53) std::cout << _ec(31);
    if (card > 52) std::cout << _ec(32);
    // show card value 
    int val = card % 13;
    if (card > 52) val = 52 - card; // jokers go negative
    if (val < 0) std::cout << "@";
    if (val > 0 && val < 10) std::cout << (val + 1);
    if (val == 0)  std::cout << "A";
    if (val == 10) std::cout << "J";
    if (val == 11) std::cout << "Q";
    if (val == 12) std::cout << "K";
    // show card suit symbol
    int ret = _setmode(_fileno(stdout), _O_U16TEXT);
    if (card < 14) std::wcout << L"\u2667" << _ec(37) << L" ";
    if (card > 13 && card < 27) std::wcout << L"\u2662" << _ec(37) << L" ";
    if (card > 26 && card < 40) std::wcout << L"\u2664" << _ec(37) << L" ";
    if (card > 39 && card < 53) std::wcout << L"\u2661" << _ec(37) << L" ";
    if (card > 52) std::wcout << _ec(37) << L"  ";
    ret = _setmode(_fileno(stdout), _O_TEXT);
}


void DisplayHand(HandInfo* hand)
{
    RankHand(hand);
    for (int card : hand->cards) Display(card);
    std::cout << "  rank: " << PokerHandName[hand->rank] << "\n";
}


int main()
{
    sIntro intro;
    intro.RunAnimatedSequence();

    // 80 char width console
    // todo: initialize players screen position
    // todo: fuck around with SetConsoleCursorPosition(hConsole, <player_n>_hand.pos); ie. redraw in the same position
    // todo: re-design screen layout (do that shit you're not supposted to do with the console)
    // ascii art the crap out of it
    // todo: ai this bish


    // setup the deck and game
    //unsigned seed = (unsigned)std::chrono::system_clock::now().time_since_epoch().count();
    //std::default_random_engine rng(seed);
    Deal();
    
    
    /* Debug testing like these values. 
    
 // A,  2,  3,  4,  5,  6,  7,  8,  9, 10,  J,  Q,  K        // 0=>12 modulo(13) value mapping  

        1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13,   // club grouping
       14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26,   // diamond grouping
       27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39,   // spade grouping
       40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52,   // heart grouping
       53, 54, 55                                            // three wildcards
    
    player2_hand.cards[0] = 9; 
    player2_hand.cards[1] = 10;
    player2_hand.cards[2] = 55;
    player2_hand.cards[3] = 54;
    player2_hand.cards[4] = 53;
    */


    std::cout << "Dealer: "; DisplayHand(&dealer_hand);
    std::cout << "Player: "; DisplayHand(&player2_hand);
    
    // start main loop
    char ch;
    int num;
    bool quit = false;
    int draw_round = 0;
    while (!quit)
    {
        std::cout << "\n[" << _ec(33) << "d" << _ec(37) << "]raw card(s), [" 
                           << _ec(33) << "r" << _ec(37) << "]eload, ["
                           << _ec(33) << "q" << _ec(37) << "]uit ";
        std::cin >> ch;
        if (ch == 'r') {
            system("cls");
            Deal();
            std::cout << "Dealer: "; DisplayHand(&dealer_hand); 
            std::cout << "Player: "; DisplayHand(&player2_hand);
            draw_round = 0;
        }
        if (ch == 'q') quit = true;
        if (ch == 'd' && draw_round < 1) {
            draw_round++;
            std::cout << "Number of Cards (" << _ec(33) << "1=>5" << _ec(37) << ") ";
            std::vector<int> card_ids;
            std::cin >> num;
            for (int i = 0; i < num; i++)
            {
                std::cout << "Which Card (" << _ec(33) << "0=>4" << _ec(37) << ") ";
                int which;
                std::cin >> which;
                card_ids.push_back(which);
            }
            // update with new cards, display and continue
            for (int id : card_ids)
            {
                player2_hand.cards[id] = deck_ids[deal_index++];
            }
            std::cout << "Player: "; DisplayHand(&player2_hand);           
        }
    }
   
    // todo: when checking win on a tie, suit matters.
    //       just compare high card on straight and flush
    //       or compare high card % 13 for others 
    //       remember to consider the Ace special case

    return 0;
}
