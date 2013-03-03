/************************************************************************/
/* AusBubble:                                                           */
/* An open-source RF jammer designed to work in the 2.4 GHz Wi-Fi       */
/* frequency block.                                                     */
/*                                                                      */
/* ui.h                                                                 */
/*                                                                      */
/* Will Robertson <aliask@gmail.com>                                    */
/* Nick D'Ademo <nickdademo@gmail.com>                                  */
/*                                                                      */
/* Copyright (c) 2012 Will Robertson, Nick D'Ademo                      */
/*                                                                      */
/* Permission is hereby granted, free of charge, to any person          */
/* obtaining a copy of this software and associated documentation       */
/* files (the "Software"), to deal in the Software without restriction, */
/* including without limitation the rights to use, copy, modify, merge, */
/* publish, distribute, sublicense, and/or sell copies of the Software, */
/* and to permit persons to whom the Software is furnished to do so,    */
/* subject to the following conditions:                                 */
/*                                                                      */
/* The above copyright notice and this permission notice shall be       */
/* included in all copies or substantial portions of the Software.      */
/*                                                                      */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF   */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND                */
/* NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS  */
/* BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN   */
/* ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN    */
/* CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE     */
/* SOFTWARE.                                                            */
/*                                                                      */
/************************************************************************/

#ifndef UI_H
#define UI_H

#include "Includes.h"
#include "SSD1306_OLED.h"
#include "Jammer.h"

/* Enumerations */
typedef enum {
    ButtonNone      = 0,
    ButtonUp        = 1 << 0,
    ButtonDown      = 1 << 1,
    ButtonLeft      = 1 << 2,
    ButtonRight     = 1 << 3,
    ButtonSelect    = 1 << 4
} buttonStates;

typedef enum {
    DisclaimerScreen = 0,
    HomeScreen,
    SynthScreen
} fsmStates;

class UI
{
    public:
        static void draw(fsmStates state);
        static void splash(const char* text, int duration_ms);
        static void doMenu(int buttons);
        static void setToggle(bool state);
        static fsmStates currentState;
    private:
        static void safeString(const char *dataPointer, unsigned char row, unsigned char xPos);
        static void safeFont57(char ascii, unsigned char row, unsigned char xPos);
        static void centredString(const char *stringPointer, unsigned char line);
        static void toggleSetting(int index);
        static void drawHomescreen(void);
        static void drawStep(int line, double stepSize);
        static void drawAlgorithm(int line, ScanAlgorithms_t algorithm);
        static void drawDisclaimer(void);
        static void drawSynthMenu(void);
        static void doDisclaimer(buttonStates action);
        static void doSynthMin(buttonStates action);
        static void doSynthMax(buttonStates action);
        static void doRate(buttonStates action);
        static void doAlgorithm(buttonStates action);
        static void doStepSize(buttonStates action);
        static void doSynthMenu(buttonStates action);
        static void doHomeScreen(buttonStates action);
        static int cursorPos;
        static bool isInSetting;
        static bool isSplashActive;
};

#endif
