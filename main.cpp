#include <Windows.h>        // GetStdHandle() and HANDLE
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

HANDLE hConsole;

std::vector<int> deck_ids = 
{
 // A,  2,  3,  4,  5,  6,  7,  8,  9, 10,  J,  Q,  K        // 0=>12 modulo(13) value mapping  
        1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13,   // club grouping
       14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26,   // diamond grouping
       27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39,   // spade grouping
       40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52    // heart grouping
};
unsigned deal_index = 0;                                     // sequential iteration (todo: auto bounds wrapping??? ((n) % 52)
std::default_random_engine rng;                              // random engine instance
const int max_players = 3;                                   // single deck game (shuffle beginning of each round)

const char* PokerHandName[27] =
{                                                                
    "high card", "", "", "",       // 0
    "one pair", "", "", "",        // 4        (2(2))
    "two pair", "", "", "",        // 8        (2(4))
    "three of a kind", "",         // 12       (2(6))
    "straight",                    // 14        
    "flush",                       // 15  
    "full house", "", "", "",      // 16       (2(8))
    "flush", "", "", "",           // 20    
    "four of a kind",              // 24       (2(12))
    "staight flush",               // 25         
    "royal straight flush"         // 26        
};

struct HandInfo
{
    int cards[5];                 // actual card ID from the deck
    int highest_card_index;       // used on zero ranking
    int rank;                     // used for determining hand strength
    COORD pos;                    // this players screen position 
};
HandInfo dealer_hand  = { 0 };    // top center
HandInfo player1_hand = { 0 };    // right
HandInfo player2_hand = { 0 };    // bottom center
HandInfo player3_hand = { 0 };    // left



static void Deal()
{
    deal_index = 0;
    std::shuffle(deck_ids.begin(), deck_ids.end(), rng);
    std::shuffle(deck_ids.begin(), deck_ids.end(), rng);
    std::shuffle(deck_ids.begin(), deck_ids.end(), rng);

    for (int i = 0; i < 5; ++i)
    {
        player2_hand.cards[i] = deck_ids.at(deal_index++); 
        dealer_hand.cards[i] = deck_ids.at(deal_index++);
    }

    player2_hand.highest_card_index = 0;
    player2_hand.rank = 0;
    dealer_hand.highest_card_index = 0;
    dealer_hand.rank = 0;
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
    //                      (25)   |         (straight flush)
    //                      (26)   |      (royal straight flush)

    hand->highest_card_index = 0;
    hand->rank = 0;
    bool flush_found    = false;
    bool straight_found = false;
    bool make_ace_high  = false;

    // find high card ---------------------------------------------------------------------------
    for (int i = 0; i < 5; i++)
    {
        int a = hand->highest_card_index % 13;
        int b = hand->cards[i] % 13;
        if (a == 0) a = 13;
        if (b == 0) b = 13;
        if (b > a)  hand->highest_card_index = hand->cards[i];
        else if (b == a)
        {   // choose the higher suit of this card value
            if (hand->cards[i] > hand->highest_card_index)
                hand->highest_card_index = hand->cards[i];
        }
    }

    // process rank solvable by comparison -----------------------------------------------------------
    for (int i = 0; i < 5; i++) 
    {
        for (int j = 0; j < 5; j++)
            if (i != j && ((hand->cards[j] % 13) == (hand->cards[i] % 13)))
                hand->rank++;
    }   
    hand->rank *= 2;                   // double result to make room for additional ranks
    if (hand->rank > 0) return;        // early out (can not be straight or flush)
    

    // continue evaluation checking for flush hand ---------------------------------------------------
    std::bitset<4> suit_bitset;
    for (int i = 0; i < 5; i++)
    {
        if (hand->cards[i] < 14) suit_bitset.set(0);                            // clubs
        if (hand->cards[i] > 13 && hand->cards[i] < 27) suit_bitset.set(1);     // diamonds
        if (hand->cards[i] > 26 && hand->cards[i] < 40) suit_bitset.set(2);     // spades
        if (hand->cards[i] > 39) suit_bitset.set(3);                            // hearts
    }
    if (suit_bitset.count() == 1) flush_found = true;
    
    // check for straight ----------------------------------------------------------------------------
    std::sort(hand->cards, hand->cards + 5, 
        [](const int& first, const int& second) -> bool
        {   // keeping connection to deck representation (handle Ace later)
            int a = first % 13;
            int b = second % 13;
            return a < b;
        }
    );
    straight_found = true; //default true for sequential test
    if ((hand->cards[4] % 13 == 12) && (hand->cards[0] % 13 == 0))
    {   // King is present. swap Ace to the back.
        make_ace_high = true;
        std::rotate(&hand->cards[0], &hand->cards[0] + 1, &hand->cards[5]);
    }
    for (int i = 0; i < 4; i++)
    {
        int a = hand->cards[i] % 13;
        int b = hand->cards[i + 1] % 13;
        if (make_ace_high)
        {
            if (a == 0) a = 13;
            if (b == 0) b = 13;
        }
        if( b - a != 1 )
        {
            straight_found = false;
        }
    }

    // final rank determination from gathered information --------------------------------------------
    if (straight_found && flush_found && (hand->cards[4] % 13 == 0)) hand->rank = 26; // royal flush
    else if (straight_found && flush_found) hand->rank = 25;                          // straight flush
    else if (flush_found) hand->rank = 15;                                            // flush
    else if (straight_found) hand->rank = 14;                                         // straight
}


void Display(int card)
{   // set color
    if (card < 14) std::cout << _ec(34);
    if (card > 13 && card < 27) std::cout << _ec(31);
    if (card > 26 && card < 40) std::cout << _ec(34);
    if (card > 39) std::cout << _ec(31);
    // show card value 
    int val = card % 13;
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
    if (card > 39) std::wcout << L"\u2661" << _ec(37) << L" ";
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
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    // 80 char width console
    // todo: initialize players screen position
    // todo: fuck around with SetConsoleCursorPosition(hConsole, <player_n>_hand.pos); ie. redraw in the same position
    // todo: re-design screen layout (do that shit you're not supposted to do with the console)
    // ascii art the crap out of it
    // todo: ai this bish


    // setup the deck and game
    unsigned seed = (unsigned)std::chrono::system_clock::now().time_since_epoch().count();
    std::default_random_engine rng(seed);
    Deal();
    
    /*
    // debug testing like this. cards are index based
    player2_hand.cards[0] = 0; // ace clubs
    player2_hand.cards[1] = 1; // two clubs
    player2_hand.cards[2] = 2; // three clubs
    player2_hand.cards[3] = 3; // etc...
    player2_hand.cards[4] = 4;
    */


    std::cout << "Dealer: "; DisplayHand(&dealer_hand);
    std::cout << "Player: "; DisplayHand(&player2_hand);
    
    // start main loop
    char ch;
    int num;
    bool quit = false;
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
        }
        if (ch == 'q') quit = true;
        if (ch == 'd') {
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
                if (deal_index == 52) { deal_index = 0; std::shuffle(deck_ids.begin(), deck_ids.end(), rng); }
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
