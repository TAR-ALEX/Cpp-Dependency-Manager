// BSD 3-Clause License

// Copyright (c) 2022, Liza Tarasova
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:

// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.

// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.

// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#pragma once

#ifdef __WIN32__
    #include <curses.h>
#else
    #include <termios.h>
    #include <unistd.h>
#endif

#include <iostream>

namespace estd {
    //declaring variables to input with the help of arrow, space, and enter keys
    const int KeY_UP = 72;
    const int KeY_LEFT = 75;
    const int KeY_RIGHT = 77;
    const int KeY_DOWN = 80;
    const int KeY_SPACE = ' ';
    const int KeY_ENTER = 13;

#ifndef __WIN32__
    char getch(void) {
        char buf = 0;
        struct termios old = {0};
        fflush(stdout);
        if (tcgetattr(0, &old) < 0) perror("tcsetattr()");
        old.c_lflag &= ~ICANON;
        old.c_lflag &= ~ECHO;
        old.c_cc[VMIN] = 1;
        old.c_cc[VTIME] = 0;
        if (tcsetattr(0, TCSANOW, &old) < 0) perror("tcsetattr ICANON");
        if (read(0, &buf, 1) < 0) perror("read()");
        old.c_lflag |= ICANON;
        old.c_lflag |= ECHO;
        if (tcsetattr(0, TCSADRAIN, &old) < 0) perror("tcsetattr ~ICANON");

        if (buf == 'A') return KeY_UP;
        if (buf == 'B') return KeY_DOWN;
        if (buf == 'C') return KeY_RIGHT;
        if (buf == 'D') return KeY_LEFT;
        if (buf == '\n') return KeY_ENTER;
        return buf;
    }
#endif

    //moves cursor to the position (x, y)
    std::string moveCursor(int x, int y) { return "\033[" + std::to_string(y) + ";" + std::to_string(x) + "H"; }

    //detects which key was pressed
    char getArrowKeyPress() {
        int c = getch();
        if (c == 0 || c == 224) { c = getch(); }
        switch (c) {
            case KeY_UP: return 'U';
            case KeY_DOWN: return 'D';
            case KeY_LEFT: return 'L';
            case KeY_RIGHT: return 'R';
            case KeY_SPACE: return ' ';
            case KeY_ENTER: return 'E';
            default: return c; break;
        }
    }

    //clears a full line in the terminal
    std::string clearLine = "\u001b[2K";
    //clears everything on a line before the cursor
    std::string clearLineBeforeCursor = "\u001b[1K";
    //clears everything on a line before after the cursor
    std::string clearLineAfterCursor = "\u001b[0K";

    //clears the screen before the cursor, everything after will remain
    std::string clearBeforeCursor = "\u001b[1J";
    //clears the screen after the cursor, everything before will remain
    std::string clearAfterCursor = "\u001b[0J";

    //clears the screen
    std::string clearScreen = moveCursor(0, 0) + clearAfterCursor;
    //Select RGB foreground color
    std::string setTextColor(int r, int g, int b) {
        return "\033[38;2;" + std::to_string(r) + ";" + std::to_string(g) + ";" + std::to_string(b) + "m";
        //Select RGB foreground color
        //\033[48; 2; <r>; <g>; <b>m     #Select RGB background color
    }
    //Select RGB background color
    std::string setBackgroundColor(int r, int g, int b) {
        return "\033[48;2;" + std::to_string(r) + ";" + std::to_string(g) + ";" + std::to_string(b) +
               "m"; //Select RGB background color
    }

    //clears all of the textSettings
    std::string clearSettings = "\033[0m";
} // namespace estd
