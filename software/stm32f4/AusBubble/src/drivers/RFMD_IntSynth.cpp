/************************************************************************/
/* AusBubble:                                                           */
/* An open-source RF jammer designed to operate in the 2.4 GHz Wi-Fi    */
/* frequency block.                                                     */
/*                                                                      */
/* RFMD_IntSynth.cpp                                                    */
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

#include "RFMD_IntSynth.h"

/* Initialize static members */
int RFMD_IntSynth::lodiv = 0;
int RFMD_IntSynth::fbkdiv = 0;
int RFMD_IntSynth::n = 0;
uint16_t RFMD_IntSynth::numlsb = 0;
uint16_t RFMD_IntSynth::nummsb = 0;
uint64_t RFMD_IntSynth::freq_Hz = 0;
uint64_t RFMD_IntSynth::freq_Hz_prev = 0;
int32_t RFMD_IntSynth::freq_delta_Hz = 0;
int32_t RFMD_IntSynth::freq_delta_Hz_prev = 0;

void RFMD_IntSynth::HWInit(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    // SCLK
    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin     = SYNTH_SCLK_PIN;
    GPIO_InitStructure.GPIO_Mode    = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType   = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed   = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd    = GPIO_PuPd_NOPULL;
    GPIO_Init(SYNTH_SCLK_PORT, &GPIO_InitStructure);
    // SDATA
    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin     = SYNTH_SDATA_PIN;
    GPIO_InitStructure.GPIO_Mode    = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType   = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed   = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd    = GPIO_PuPd_NOPULL;
    GPIO_Init(SYNTH_SDATA_PORT, &GPIO_InitStructure);
    // ENX
    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin     = SYNTH_ENX_PIN;
    GPIO_InitStructure.GPIO_Mode    = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType   = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed   = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd    = GPIO_PuPd_NOPULL;
    GPIO_Init(SYNTH_ENX_PORT, &GPIO_InitStructure);
    // RESETX
    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin     = SYNTH_RESETX_PIN;
    GPIO_InitStructure.GPIO_Mode    = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType   = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed   = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd    = GPIO_PuPd_NOPULL;
    GPIO_Init(SYNTH_RESETX_PORT, &GPIO_InitStructure);
    // GPO1/ADD1
    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin     = SYNTH_GPO1ADD1_PIN;
    GPIO_InitStructure.GPIO_Mode    = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_OType   = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed   = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd    = GPIO_PuPd_NOPULL;
    GPIO_Init(SYNTH_GPO1ADD1_PORT, &GPIO_InitStructure);
    // GPO2/ADD2
    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin     = SYNTH_GPO2ADD2_PIN;
    GPIO_InitStructure.GPIO_Mode    = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_OType   = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed   = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd    = GPIO_PuPd_NOPULL;
    GPIO_Init(SYNTH_GPO2ADD2_PORT, &GPIO_InitStructure);
    // GPO3/FM
    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin     = SYNTH_GPO3FM_PIN;
    GPIO_InitStructure.GPIO_Mode    = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_OType   = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed   = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd    = GPIO_PuPd_NOPULL;
    GPIO_Init(SYNTH_GPO3FM_PORT, &GPIO_InitStructure);
    // GPO4/LD/DO
    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin     = SYNTH_GPO4LDDO_PIN;
    GPIO_InitStructure.GPIO_Mode    = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_OType   = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed   = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd    = GPIO_PuPd_NOPULL;
    GPIO_Init(SYNTH_GPO4LDDO_PORT, &GPIO_InitStructure);
    // ENBL/GPO5
    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin     = SYNTH_ENBLGPO5_PIN;
    GPIO_InitStructure.GPIO_Mode    = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType   = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed   = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd    = GPIO_PuPd_NOPULL;
    GPIO_Init(SYNTH_ENBLGPO5_PORT, &GPIO_InitStructure);
    // MODE/GPO6
    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin     = SYNTH_MODEGPO6_PIN;
    GPIO_InitStructure.GPIO_Mode    = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType   = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed   = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd    = GPIO_PuPd_NOPULL;
    GPIO_Init(SYNTH_MODEGPO6_PORT, &GPIO_InitStructure);
    // Set low
    GPIO_ResetBits(SYNTH_MODEGPO6_PORT, SYNTH_MODEGPO6_PIN);
}

void RFMD_IntSynth::Init(void)
{
    /* Set initial state of GPIO pins and perform hardware reset */
    // RESETX=0 (hardware reset START)
    GPIO_ResetBits(SYNTH_RESETX_PORT, SYNTH_RESETX_PIN);
    // ENX=1
    GPIO_SetBits(SYNTH_ENX_PORT, SYNTH_ENX_PIN);
    // SCLK=0
    GPIO_ResetBits(SYNTH_SCLK_PORT, SYNTH_SCLK_PIN);
    // SDATA=0
    GPIO_ResetBits(SYNTH_SDATA_PORT, SYNTH_SDATA_PIN);
    // ENBL=0
    GPIO_ResetBits(SYNTH_ENBLGPO5_PORT, SYNTH_ENBLGPO5_PIN);
    // RESETX=1 (hardware reset END)
    GPIO_SetBits(SYNTH_RESETX_PORT, SYNTH_RESETX_PIN);

    /* Software Reset */
    Write((REG_SDI_CTRL<<16) | (1<<SHIFT_SDI_CTRL__RESET));      // [1] When this bit is taken high the part is reset

    /* Configure device */
    // Set GPO4 to output LOCK flag
    Write((REG_GPO<<16) | (1<<SHIFT_GPO__LOCK));                 // [0] Sends LOCK flag to GPO4
    // Bypass the mixer
    Write((REG_DEV_CTRL<<16) | (1<<SHIFT_DEV_CTRL__BYPASS));     // [1] If high, offsets mixer so that LO signal can be viewed at mixer output

    /* Enable Device */
    SetEnabled(true);

    /* Set frequency to 2450 MHz (wait for PLL lock, no frequency modulation) */
    SetFreq(2450000000UL, true, false);
}

void RFMD_IntSynth::SetEnabled(bool enable)
{
    #if USE_SW_CONTROL
        Write((REG_SDI_CTRL<<16) | (1<<SHIFT_SDI_CTRL__SIPIN) |       // [15] 1=ENBL and MODE pins are ignored and become available as GPO5 and GPO6
                                   (enable<<SHIFT_SDI_CTRL__ENBL));   // [14] If sipin=1 this field will replace the functionality of the ENBL pin
    #else
        if(enable)
            GPIO_WriteBit(SYNTH_ENBLGPO5_PORT, SYNTH_ENBLGPO5_PIN, Bit_SET);
        else
            GPIO_WriteBit(SYNTH_ENBLGPO5_PORT, SYNTH_ENBLGPO5_PIN, Bit_RESET);
    #endif
}

void RFMD_IntSynth::Write(uint32_t dataBits)
{
    // Initialize count variable to zero
    uint8_t count = 0;

    // Pulse SCLK
    GPIO_SetBits(SYNTH_SCLK_PORT, SYNTH_SCLK_PIN);
    GPIO_ResetBits(SYNTH_SCLK_PORT, SYNTH_SCLK_PIN);

    // ENX=0
    GPIO_ResetBits(SYNTH_ENX_PORT, SYNTH_ENX_PIN);

    // Pulse SCLK (for don't care SDATA bit)
    GPIO_SetBits(SYNTH_SCLK_PORT, SYNTH_SCLK_PIN);
    GPIO_ResetBits(SYNTH_SCLK_PORT, SYNTH_SCLK_PIN);

    // SDATA=0 (WRITE MODE)
    GPIO_ResetBits(SYNTH_SDATA_PORT, SYNTH_SDATA_PIN);
    asm volatile("nop");

    // Pulse SCLK
    GPIO_SetBits(SYNTH_SCLK_PORT, SYNTH_SCLK_PIN);
    GPIO_ResetBits(SYNTH_SCLK_PORT, SYNTH_SCLK_PIN);

    // Put 23 bits on SDATA line (MSB first)
    while(count < 23)
    {
        if(dataBits & (0x00400000 >> count))
        {
            // SDATA=1
            GPIO_SetBits(SYNTH_SDATA_PORT, SYNTH_SDATA_PIN);
            asm volatile("nop");

            // Pulse SCLK
            GPIO_SetBits(SYNTH_SCLK_PORT, SYNTH_SCLK_PIN);
            GPIO_ResetBits(SYNTH_SCLK_PORT, SYNTH_SCLK_PIN);
        }
        else
        {
            // SDATA=0
            GPIO_ResetBits(SYNTH_SDATA_PORT, SYNTH_SDATA_PIN);
            asm volatile("nop");

            // Pulse SCLK
            GPIO_SetBits(SYNTH_SCLK_PORT, SYNTH_SCLK_PIN);
            GPIO_ResetBits(SYNTH_SCLK_PORT, SYNTH_SCLK_PIN);
        }
        count++;
    }

    // ENX=1
    GPIO_SetBits(SYNTH_ENX_PORT, SYNTH_ENX_PIN);

    // Pulse SCLK
    GPIO_SetBits(SYNTH_SCLK_PORT, SYNTH_SCLK_PIN);
    GPIO_ResetBits(SYNTH_SCLK_PORT, SYNTH_SCLK_PIN);
}

void RFMD_IntSynth::SendAddress(bool write, uint8_t address)
{
    // Initialize count variable to zero
    uint8_t count = 0;

    // Pulse SCLK
    GPIO_WriteBit(SYNTH_SCLK_PORT, SYNTH_SCLK_PIN, Bit_SET);
    GPIO_WriteBit(SYNTH_SCLK_PORT, SYNTH_SCLK_PIN, Bit_RESET);

    // ENX=0
    GPIO_WriteBit(SYNTH_ENX_PORT, SYNTH_ENX_PIN, Bit_RESET);

    // Pulse SCLK (for don't care SDATA bit)
    GPIO_WriteBit(SYNTH_SCLK_PORT, SYNTH_SCLK_PIN, Bit_SET);
    GPIO_WriteBit(SYNTH_SCLK_PORT, SYNTH_SCLK_PIN, Bit_RESET);

    // R=0 (WRITE MODE)
    if(write)
    {
        // SDATA=0
        GPIO_WriteBit(SYNTH_SDATA_PORT, SYNTH_SDATA_PIN, Bit_RESET);
        asm volatile("nop");

        // Pulse SCLK
        GPIO_WriteBit(SYNTH_SCLK_PORT, SYNTH_SCLK_PIN, Bit_SET);
        GPIO_WriteBit(SYNTH_SCLK_PORT, SYNTH_SCLK_PIN, Bit_RESET);
    }
    // R=1 (READ MODE)
    else
    {
        // SDATA=1
        GPIO_WriteBit(SYNTH_SDATA_PORT, SYNTH_SDATA_PIN, Bit_SET);
        asm volatile("nop");

        // Pulse SCLK
        GPIO_WriteBit(SYNTH_SCLK_PORT, SYNTH_SCLK_PIN, Bit_SET);
        GPIO_WriteBit(SYNTH_SCLK_PORT, SYNTH_SCLK_PIN, Bit_RESET);
    }

    // Put 7 bits on SDATA line (MSB first)
    while(count < 7)
    {
        if(address & (0x40 >> count))
        {
            // SDATA=1
            GPIO_WriteBit(SYNTH_SDATA_PORT, SYNTH_SDATA_PIN, Bit_SET);
            asm volatile("nop");

            // Pulse SCLK
            GPIO_WriteBit(SYNTH_SCLK_PORT, SYNTH_SCLK_PIN, Bit_SET);
            GPIO_WriteBit(SYNTH_SCLK_PORT, SYNTH_SCLK_PIN, Bit_RESET);
        }
        else
        {
            // SDATA=0
            GPIO_WriteBit(SYNTH_SDATA_PORT, SYNTH_SDATA_PIN, Bit_RESET);
            asm volatile("nop");

            // Pulse SCLK
            GPIO_WriteBit(SYNTH_SCLK_PORT, SYNTH_SCLK_PIN, Bit_SET);
            GPIO_WriteBit(SYNTH_SCLK_PORT, SYNTH_SCLK_PIN, Bit_RESET);
        }
        count++;
    }

    // If reading device, insert extra clock edges
    if(!write)
    {
        asm volatile("nop");
        GPIO_WriteBit(SYNTH_SCLK_PORT, SYNTH_SCLK_PIN, Bit_SET);
        GPIO_WriteBit(SYNTH_SCLK_PORT, SYNTH_SCLK_PIN, Bit_RESET);
        GPIO_WriteBit(SYNTH_SCLK_PORT, SYNTH_SCLK_PIN, Bit_SET);
    }
}

void RFMD_IntSynth::SendData(uint16_t data)
{
    // Initialize count variable to zero
    uint8_t count = 0;

    // Put 16 bits on SDATA line (MSB first)
    while (count < 16)
    {
        if (data & (0x8000 >> count))
        {
            // SDATA=1
            GPIO_WriteBit(SYNTH_SDATA_PORT, SYNTH_SDATA_PIN, Bit_SET);
            asm volatile("nop");

            // Pulse SCLK
            GPIO_WriteBit(SYNTH_SCLK_PORT, SYNTH_SCLK_PIN, Bit_SET);
            GPIO_WriteBit(SYNTH_SCLK_PORT, SYNTH_SCLK_PIN, Bit_RESET);
        }
        else
        {
            // SDATA=0
            GPIO_WriteBit(SYNTH_SDATA_PORT, SYNTH_SDATA_PIN, Bit_RESET);
            asm volatile("nop");

            // Pulse SCLK
            GPIO_WriteBit(SYNTH_SCLK_PORT, SYNTH_SCLK_PIN, Bit_SET);
            GPIO_WriteBit(SYNTH_SCLK_PORT, SYNTH_SCLK_PIN, Bit_RESET);
        }
        count++;
    }

    // ENX=1
    GPIO_WriteBit(SYNTH_ENX_PORT, SYNTH_ENX_PIN, Bit_SET);

    // Pulse SCLK
    GPIO_WriteBit(SYNTH_SCLK_PORT, SYNTH_SCLK_PIN, Bit_SET);
    GPIO_WriteBit(SYNTH_SCLK_PORT, SYNTH_SCLK_PIN, Bit_RESET);
}

uint16_t RFMD_IntSynth::ReceiveData(void)
{
    // Initialize local variables
    uint8_t count = 0;
    uint16_t read_data = 0;

    // Set SDATA as input
    SYNTH_SDATA_PORT->MODER  &= ~(GPIO_MODER_MODER0 << (SYNTH_SDATA_PIN_N * 2));
    SYNTH_SDATA_PORT->MODER |= (((uint32_t)GPIO_Mode_IN) << (SYNTH_SDATA_PIN_N * 2));

    while (count < 16)
    {
        // Build data word, MSB read back first
        asm volatile("nop");
        GPIO_WriteBit(SYNTH_SCLK_PORT, SYNTH_SCLK_PIN, Bit_RESET);
        read_data |= ((SYNTH_SDATA_PORT->IDR & SYNTH_SDATA_PIN) != 0) << (15 - count);
        asm volatile("nop");
        GPIO_WriteBit(SYNTH_SCLK_PORT, SYNTH_SCLK_PIN, Bit_SET);
        count++;
    }

    // Reception complete
    GPIO_WriteBit(SYNTH_ENX_PORT, SYNTH_ENX_PIN, Bit_SET);
    asm volatile("nop");
    GPIO_WriteBit(SYNTH_SCLK_PORT, SYNTH_SCLK_PIN, Bit_RESET);

    // Set SDATA as output
    SYNTH_SDATA_PORT->MODER  &= ~(GPIO_MODER_MODER0 << (SYNTH_SDATA_PIN_N * 2));
    SYNTH_SDATA_PORT->MODER |= (((uint32_t)GPIO_Mode_OUT) << (SYNTH_SDATA_PIN_N * 2));

    return read_data;
}

uint16_t RFMD_IntSynth::Read(uint8_t address)
{
    SendAddress(false, address);
    return ReceiveData();
}

bool RFMD_IntSynth::isPLLLocked(void)
{
    return ((SYNTH_GPO4LDDO_PORT->IDR & SYNTH_GPO4LDDO_PIN) == SYNTH_GPO4LDDO_PIN);
}

void RFMD_IntSynth::SetFreqLO(uint64_t f_lo_Hz, bool waitForPLLLock)
{
    /* Register calculations taken from RFMD Programming Guide
    Source: http://www.rfmd.com/CS/Documents/IntegratedSyntMixerProgrammingGuide.pdf */
    int n_lo                = log2f((float)(F_VCO_MAX_HZ/f_lo_Hz));
    RFMD_IntSynth::lodiv    = 1<<n_lo;
    uint64_t f_vco          = lodiv*f_lo_Hz;

    /* If the VCO frequency is above 3.2GHz it is necessary to set the prescaler to /4
    and charge pump leakage to 3 for the CT_cal to work correctly */
    if(f_vco > CP_THRESHOLD_VCO_FREQ_HZ)
    {
        RFMD_IntSynth::fbkdiv = FBKDIV_4;
        Write((REG_LF<<16) | (1<<SHIFT_LF__LFACT)      // Active loop filter enable, 1=active 0=passive
                           | (32<<SHIFT_LF__P2CPDEF)   // Charge pump setting. If p2_kv_en=1 this value sets charge pump current during KV compensation measurement. If p2_kv_en=0, this value is used at all times. Default value is 93uA.
                           | (32<<SHIFT_LF__P1CPDEF)   // Charge pump setting. If p1_kv_en=1 this value sets charge pump current during KV compensation measurement. If p1_kv_en=0, this value is used at all times. Default value is 93uA.
                           | (3<<SHIFT_LF__PLLCPL));   // Charge pump leakage settings
    }
    else
    {
        RFMD_IntSynth::fbkdiv = FBKDIV_2;
        Write((REG_LF<<16) | (1<<SHIFT_LF__LFACT)      // Active loop filter enable, 1=active 0=passive
                           | (32<<SHIFT_LF__P2CPDEF)   // Charge pump setting. If p2_kv_en=1 this value sets charge pump current during KV compensation measurement. If p2_kv_en=0, this value is used at all times. Default value is 93uA.
                           | (32<<SHIFT_LF__P1CPDEF)   // Charge pump setting. If p1_kv_en=1 this value sets charge pump current during KV compensation measurement. If p1_kv_en=0, this value is used at all times. Default value is 93uA.
                           | (2<<SHIFT_LF__PLLCPL));   // Charge pump leakage settings
    }
    float n_div             = (float)f_vco/(float)(RFMD_IntSynth::fbkdiv*F_REFERENCE_HZ);
    RFMD_IntSynth::n        = n_div;
    RFMD_IntSynth::nummsb   = (1<<16)*(n_div-n);
    RFMD_IntSynth::numlsb   = (1<<8)*((1<<16)*(n_div-n)-RFMD_IntSynth::nummsb);

    /* Set N divider, LO path divider and feedback divider */
    Write((REG_P1_FREQ1<<16) | (n<<SHIFT_P1_FREQ1__P1N) |                                   // Path 1 VCO divider integer value
                               ((int)log2(RFMD_IntSynth::lodiv)<<SHIFT_P1_FREQ1__P1LODIV) | // Path 1 LO path divider setting: divide by 2^n (i.e. divide by 1 to divide by 32). 110 and 111 are reserved
                               ((RFMD_IntSynth::fbkdiv>>1)<<SHIFT_P1_FREQ1__P1PRESC));      // Path 1 VCO PLL feedback path divider setting: 01 = divide by 2, 10 = divide by 4 (00 and 11 are reserved)
    Write((REG_P1_FREQ2<<16) | RFMD_IntSynth::nummsb);                                      // Path 1 N divider numerator value, most significant 16 bits
    Write((REG_P1_FREQ3<<16) | RFMD_IntSynth::numlsb);                                      // Path 1 N divider numerator value, least significant 8 bits

    /* Reset FMOD (so that the desired frequency is set if frequency modulation is/was being used)
    Note: This register sets the Frequency Deviation applied to frac-N */
    Write((REG_FMOD<<16) | 0);  // [15:0] Frequency Deviation applied to frac-N, functionality determined by modstep and mod_setup

    // Re-lock the PLL
    Write((REG_PLL_CTRL<<16) | (1<<SHIFT_PLL_CTRL__DIVBY) |  // [15] Force reference divider to divide by 1
                               (8<<SHIFT_PLL_CTRL__TVCO) |   // [10:6] VCO warm-up time. warm-up time [s] = tvco * 1/[fref*256]
                               (1<<SHIFT_PLL_CTRL__LDEN) |   // [5] Enable lock detector circuitry
                               (1<<SHIFT_PLL_CTRL__RELOK));  // [3] Self Clearing Bit. When this bit is set high it triggers a relock of the PLL and then clears

    // Wait for PLL lock
    while(waitForPLLLock && !isPLLLocked());
}

void RFMD_IntSynth::SetFreq(uint64_t freq_Hz, bool waitForPLLLock, bool useModulation)
{
    static int16_t cur_fmod = 0;
    static int16_t fmod_step = 0;
    static int16_t fmod_lower_bound = 0;
    static int16_t fmod_upper_bound = 0;

    RFMD_IntSynth::freq_Hz = freq_Hz;
    RFMD_IntSynth::freq_delta_Hz = freq_Hz - freq_Hz_prev;

    /* FREQUENCY MODULATION */
    // Check 1: Is Frequency Modulation enabled?
    // Check 2: Is the PLL locked?
    // Check 3: Has frequency step size changed? (i.e. are current modulation settings still valid?)
    // Check 4: Is the frequency step size valid?
    // Checks 5 & 6: Can we reach the next frequency using modulation?
    if(useModulation &&
            (isPLLLocked()||!waitForPLLLock) &&
            (abs(RFMD_IntSynth::freq_delta_Hz) == abs(RFMD_IntSynth::freq_delta_Hz_prev)) &&
            (fmod_step != 0) &&
            ((cur_fmod + fmod_step) <= fmod_upper_bound) &&
            ((cur_fmod - fmod_step) >= fmod_lower_bound))
    {
        cur_fmod += fmod_step*(freq_delta_Hz > 0 ? 1 : -1);
        Write((REG_FMOD<<16) | (uint16_t)cur_fmod); // [15:0] Frequency Deviation applied to frac-N, functionality determined by modstep and mod_setup
    }
    /* SET FREQUENCY BY WRITING TO FREQ1, FREQ2, FREQ3 REGISTERS */
    else
    {
        // Set frequency
        SetFreqLO(freq_Hz, waitForPLLLock);

        /* Calculate modulation parameters */
        if(useModulation)
        {
            // Reset FMOD to 0
            cur_fmod = 0;
            // Set optimum modulation parameters depending on frequency delta
            // Note: Frequency modulation is only used for valid frequency deltas (i.e. step sizes)
            uint8_t modstep;
            GetModParams(freq_delta_Hz, modstep, fmod_step);
            // Valid frequency delta
            if(fmod_step != 0)
            {
                uint32_t n_24bit = (RFMD_IntSynth::nummsb<<8) | (RFMD_IntSynth::numlsb>>8);
                uint32_t max_fmod = 0x7FFF << modstep; // FMOD is multiplied by 2^MODSTEP before being added to frac-N
                // LOWER BOUND
                if(max_fmod <= n_24bit)
                    fmod_lower_bound = -1*0x7FFF;
                else
                    fmod_lower_bound = -1*((n_24bit & max_fmod) >> modstep);
                // UPPER BOUND
                if((n_24bit + max_fmod) <= 0xFFFFFF)
                    fmod_upper_bound = 0x7FFF;
                else
                    fmod_upper_bound = ((0xFFFFFF-n_24bit) & max_fmod) >> modstep;

                // Enable frequency modulation
                Write((REG_EXT_MOD<<16) | (1<<SHIFT_EXT_MOD__MODSETUP) |        // [15:14] Modulation is analog, on every update of modulation the frac-N responds by adding value to frac-N
                                          (modstep<<SHIFT_EXT_MOD__MODSTEP));   // [13:10] Modulation scale factor. Modulation is multiplied by 2^modstep before being added to frac-N. Maximum usable value is 8
            }
        }
    }

    RFMD_IntSynth::freq_Hz_prev = freq_Hz;
    RFMD_IntSynth::freq_delta_Hz_prev = RFMD_IntSynth::freq_delta_Hz;
}

void RFMD_IntSynth::GetModParams(int32_t freq_delta_Hz, uint8_t &modstep, int16_t &fmod_step)
{
    /*
          Fmod = MOD * (2^MODSTEP) * step_size

          where     step_size  =   (Fref * P) / (R * (2^24) * LO_DIV)

          where     Fref       =   Reference frequency
                    MODSTEP    =   Modulation scale factor                  {0,1,2,3,4,5,6,7}
                    MOD        =   Frequency deviation applied to frac-N    [-32768,32767]
                    P          =   Prescaler division ratio                 {2,4}
                    R          =   Reference division ratio                 {1,2,3,4,5,6,7}
                    LO_DIV     =   Low divider value                        {2,4,8,16,32}
    */

    // Assume P=4, R=1, LO_DIV=2

    // 1 kHz
    // f = 5 * (2^6) * ( (26e6 * 4) / (1 * (2^24) * 2) ) = 991.82 Hz
    if(abs(freq_delta_Hz) == STEP_1K_HZ)
    {
        modstep = 6;
        fmod_step = 5;
    }
    // 10 kHz
    // f = 25 * (2^7) * ( (26e6 * 4) / (1 * (2^24) * 2) ) = 9918.21 Hz
    else if(abs(freq_delta_Hz) == STEP_10K_HZ)
    {
        modstep = 7;
        fmod_step = 25;
    }
    // 25 kHz
    // f = 63 * (2^7) * ( (26e6 * 4) / (1 * (2^24) * 2) ) = 24993.90 Hz
    else if(abs(freq_delta_Hz) == STEP_25K_HZ)
    {
        modstep = 7;
        fmod_step = 63;
    }
    // 50 kHz
    // f = 63 * (2^8) * ( (26e6 * 4) / (1 * (2^24) * 2) ) = 49987.79 Hz
    else if(abs(freq_delta_Hz) == STEP_50K_HZ)
    {
        modstep = 8;
        fmod_step = 63;
    }
    // 100 kHz
    // f = 126 * (2^8) * ( (26e6 * 4) / (1 * (2^24) * 2) ) = 99975.59 Hz
    else if(abs(freq_delta_Hz) == STEP_100K_HZ)
    {
        modstep = 8;
        fmod_step = 126;
    }
    // 250 kHz
    // f = 315 * (2^8) * ( (26e6 * 4) / (1 * (2^24) * 2) ) = 249938.96 Hz
    else if(abs(freq_delta_Hz) == STEP_250K_HZ)
    {
        modstep = 8;
        fmod_step = 315;
    }
    // 500 kHz
    // f = 630 * (2^8) * ( (26e6 * 4) / (1 * (2^24) * 2) ) = 499877.93 Hz
    else if(abs(freq_delta_Hz) == STEP_500K_HZ)
    {
        modstep = 8;
        fmod_step = 630;
    }
    // 1 MHz
    // f = 1260 * (2^8) * ( (26e6 * 4) / (1 * (2^24) * 2) ) = 999755.86 Hz
    else if(abs(freq_delta_Hz) == STEP_1M_HZ)
    {
        modstep = 8;
        fmod_step = 1260;
    }
    // Unsupported frequency delta
    else
    {
        fmod_step = 0;
    }
}

void RFMD_IntSynth::GetFMODFreqLimits(uint64_t freq_Hz, int32_t freq_delta_Hz, uint64_t &f_lower_bound, uint64_t &f_upper_bound)
{
    int16_t fmod_lower_bound = 0;
    int16_t fmod_upper_bound = 0;
    int16_t fmod_step = 0;
    uint8_t modstep = 0;
    uint16_t nummsb = 0;
    uint16_t numlsb = 0;

    /* Get freq register parameters */
    RFMD_IntSynth::GetFreqRegParams(freq_Hz, nummsb, numlsb);

    /* Get modulation parameters */
    RFMD_IntSynth::GetModParams(freq_delta_Hz, modstep, fmod_step);

    /* Valid frequency delta */
    if(fmod_step != 0)
    {
        uint32_t n_24bit = (nummsb<<8) | (numlsb>>8);
        uint32_t max_fmod = 0x7FFF << modstep; // FMOD is multiplied by 2^MODSTEP before being added to frac-N

        /* LOWER BOUND */
        if(max_fmod <= n_24bit)
            fmod_lower_bound = -1*0x7FFF;
        else
            fmod_lower_bound = -1*((n_24bit & max_fmod) >> modstep);
        /* UPPER BOUND */
        if((n_24bit + max_fmod) <= 0xFFFFFF)
            fmod_upper_bound = 0x7FFF;
        else
            fmod_upper_bound = ((0xFFFFFF-n_24bit) & max_fmod) >> modstep;

        /* Calculate frequency limits */
        // Lower
        f_lower_bound = freq_Hz - abs(freq_delta_Hz)*(int)(abs(fmod_lower_bound)/fmod_step);
        // Upper
        f_upper_bound = freq_Hz + abs(freq_delta_Hz)*(int)(abs(fmod_upper_bound)/fmod_step);
    }
}

void RFMD_IntSynth::GetFreqRegParams(uint64_t f_lo_Hz, uint16_t &nummsb, uint16_t &numlsb)
{
    int lodiv = 0;
    int fbkdiv = 0;
    int n = 0;

    /* Register calculations taken from RFMD Programming Guide
    Source: http://www.rfmd.com/CS/Documents/IntegratedSyntMixerProgrammingGuide.pdf */
    int n_lo                = log2f((float)(F_VCO_MAX_HZ/f_lo_Hz));
    lodiv                   = 1<<n_lo;
    uint64_t f_vco          = lodiv*f_lo_Hz;

    /* If the VCO frequency is above 3.2GHz it is necessary to set the prescaler to /4
    and charge pump leakage to 3 for the CT_cal to work correctly */
    if(f_vco > CP_THRESHOLD_VCO_FREQ_HZ)
    {
        fbkdiv = FBKDIV_4;
    }
    else
    {
        fbkdiv = FBKDIV_2;
    }
    float n_div             = (float)f_vco/(float)(fbkdiv*F_REFERENCE_HZ);
    n                       = n_div;
    nummsb                  = (1<<16)*(n_div-n);
    numlsb                  = (1<<8)*((1<<16)*(n_div-n)-nummsb);
}
