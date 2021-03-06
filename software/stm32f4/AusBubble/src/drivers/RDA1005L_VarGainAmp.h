/************************************************************************/
/* AusBubble:                                                           */
/* An open-source RF jammer designed to operate in the 2.4 GHz Wi-Fi    */
/* frequency block.                                                     */
/*                                                                      */
/* RDA1005L_VarGainAmp.h                                                */
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

#ifndef RDA1005L_VARGAINAMP_H
#define RDA1005L_VARGAINAMP_H

#include "Includes.h"

/* Gain Limits (DO NOT MODIFY) */
#define VARGAINAMP_MAX_GAIN_LIMIT_DB    6.0     // Permanent damage to the final-stage amplifier may result if this value is too HIGH (check input power limits in datasheet)
#define VARGAINAMP_MIN_GAIN_LIMIT_DB    -13.5
#define VARGAINAMP_MAX_GAIN_DB          18.0    // Maximum rated gain of amplifier
#define VARGAINAMP_MIN_GAIN_DB          -13.5   // Minimum rated gain of amplifier

class RDA1005L_VarGainAmp
{
    public:
        static void HWInit(void);
        static void SetGain(double gain_dB);
    private:
        static void Write(uint8_t data);
};

#endif
