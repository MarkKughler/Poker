#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>                 // GetStdHandle() and HANDLE
#include <iostream>

/*
|     .          .          .          .          .          .          .          .     |
|           .          .          .          .          .          .          .          |
|                                                                                        |
|         ####          ###   ###   ####   ####       ####   ####    ###   #     #       |
|         #            #     #   #  #   #  #   #      #   #  #   #  #   #  #     #       |
|         ###    ###   #     #####  ####   #   #      #   #  ####   #####  #  #  #       |
|            #         #     #   #  #  #   #   #      #   #  #  #   #   #  #  #  #       |
|         ###           ###  #   #  #   #  ####       ####   #   #  #   #   ## ##        |
|                                                                                        |
|                    #######    ######   #      #  ########  #######                     |
|                    #      #  #      #  #    ##   #         #      #                    |
|                    #      #  #      #  #  ##     #         #      #                    |
|                    #     #   #      #  ###       ######    #     #                     |
|                    ######    #      #  # ##      #         ######                      |
|                    #         #      #  #   ##    #         #   ##                      |
|                    #          ######   #     ##  ########  #     ##                    |
|                                                                                        |
|                           with three jokers in the deck                                |
|                                                                                        |
|                                                                                        |
*/

struct sIntro
{
    HANDLE hConsole;


    void RunAnimatedSequence()
    {
        hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

        RunPokerTextSequence();
    }


    void RunPokerTextSequence()
    {   // drop down from top

        RunFiveCardSequence();
    }


    void RunFiveCardSequence()
    {   // drop down from top

        RunJokerInDeckSequence();
    }


    void RunJokerInDeckSequence()
    {  // slide in from left

        RunWaitSequence();
    }


    void RunWaitSequence()
    {
        char ch;
        std::cin >> ch;
    }

};
