// BSD 3-Clause License

// Copyright (c) 2024, Alex Tarasov
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

#if defined(_WIN32) || defined(WIN32) || defined(__CYGWIN__) || defined(__MINGW32__) || defined(__BORLANDC__)
    #include <conio.h>
#else
    #include <termios.h>
    #include <unistd.h>
#endif

#include <iostream>
#include <tuple>

namespace estd {
    //declaring variables to input with the help of arrow, space, and enter keys
    const int KeY_UP = 72;
    const int KeY_LEFT = 75;
    const int KeY_RIGHT = 77;
    const int KeY_DOWN = 80;
    const int KeY_SPACE = ' ';
    const int KeY_ENTER = 13;

#if !(defined(_WIN32) || defined(WIN32) || defined(__CYGWIN__) || defined(__MINGW32__) || defined(__BORLANDC__))
    inline char getch(void) {
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
    inline std::string moveCursor(int x, int y) { return "\033[" + std::to_string(y) + ";" + std::to_string(x) + "f"; }

    //detects which key was pressed
    inline char getArrowKeyPress() {
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
    const std::string saveCursorPosition = "\033[s";
    const std::string restoreCursorPosition = "\033[u";
    inline std::string scroll(int y) {
        if(y < 0) {return "\033[" + std::to_string(y) + "S"; }
        return "\033[" + std::to_string(y) + "T";
    }

    inline std::string resizeWindow(int x,int y) {
        return "\033[8;"+std::to_string(y)+";"+std::to_string(x)+"t";
    }
    

    //clears a full line in the terminal
    const std::string clearLine = "\u001b[2K";
    //clears everything on a line before the cursor
    const std::string clearLineBeforeCursor = "\u001b[1K";
    //clears everything on a line before after the cursor
    const std::string clearLineAfterCursor = "\u001b[0K";

    //clears the screen before the cursor, everything after will remain
    const std::string clearBeforeCursor = "\u001b[1J";
    //clears the screen after the cursor, everything before will remain
    const std::string clearAfterCursor = "\u001b[0J";

    //clears the screen
    const std::string clearScreen = moveCursor(0, 0) + clearAfterCursor;

    /**
     * Converts HSV color values to RGB.
     *
     * @param h Hue value (0-360) converted to 0-1
     * @param s Saturation value (0-100) converted to 0-1
     * @param v Value value (0-100) converted to 0-1
     * @return RGB color values as a tuple { red, green, blue }
     */
    std::tuple<int, int, int> hsv_to_rgb(double h, double s, double v) {
        // Normalize hue, saturation, and value values to range [0, 1]
        h /= 360.0;
        s /= 100.0;
        v /= 100.0;

        // Convert HSV components to normalized values
        int hi = int(h * 6);
        double f = h * 6 - hi;
        double p = v * (1 - s);
        double q = v * (1 - f * s);
        double t = v * (1 - (1 - f) * s);

        // Compute RGB values based on hue component
        int i = hi % 6;
        switch (i) {
            case 0: return {int(v * 255), int(t * 255), int(p * 255)};
            case 1: return {int(q * 255), int(v * 255), int(p * 255)};
            case 2: return {int(p * 255), int(v * 255), int(t * 255)};
            case 3: return {int(p * 255), int(q * 255), int(v * 255)}; 
            case 4: return {int(t * 255), int(p * 255), int(v * 255)};
            default: return {int(v * 255), int(p * 255), int(q * 255)}; // case 5
        }
    }


    //Select RGB foreground color
    inline std::string setTextColor(int r, int g, int b) {
        return "\033[38;2;" + std::to_string(r) + ";" + std::to_string(g) + ";" + std::to_string(b) + "m";
        //Select RGB foreground color
        //\033[48; 2; <r>; <g>; <b>m     #Select RGB background color
    }
    //Select RGB background color
    inline std::string setBackgroundColor(int r, int g, int b) {
        return "\033[48;2;" + std::to_string(r) + ";" + std::to_string(g) + ";" + std::to_string(b) +
               "m"; //Select RGB background color
    }
    //Select HSV foreground color 
    inline std::string setTextColorHSV(int h, int s, int v) {
        auto [r,g,b] = hsv_to_rgb(h,s,v);
        // std::cout << h << ", "<< s << ", "<< v << ", -- ";
        // std::cout << r << ", "<< g << ", "<< b << ", ";
        return setTextColor(r,g,b);
    }
    //Select HSV background color
    inline std::string setBackgroundColorHSV(int h, int s, int v) {
        auto [r,g,b] = hsv_to_rgb(h,s,v);
        return setBackgroundColor(r,g,b);
    }

    //clears all of the textSettings
    const std::string clearSettings = "\033[0m";
} // namespace estd
