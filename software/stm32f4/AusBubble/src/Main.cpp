/************************************************************************/
/* AusBubble:                                                           */
/* An open-source RF jammer designed to operate in the 2.4 GHz Wi-Fi    */
/* frequency block.                                                     */
/*                                                                      */
/* Main.cpp                                                             */
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

#include "Includes.h"

#include "OLED.h"
#include "UI.h"
#include "ScanAlgorithms.h"
#include "VarGainAmp.h"

/* ADC3 Data Register Address */
#define ADC3_DR_ADDRESS    ((uint32_t)0x4001224C)

/* Defines for the button hold behaviour */
#define TICK_INITIALRATE    200
#define TICK_FAST           25
#define TICK_SLOW           200
#define TICK_HOLDCOUNT      400

/* Global Variables */
__IO uint16_t gADC3ConvertedValue = 0;

/* Function Prototypes */
void prvSetupHardware(void);
void SetupJoystick(void);
void NVIC_Config(void);
void SPI1_Config(void);
void ADC3_CH12_DMA_Config(void);

/* RTOS millisecond delay function */
void DelayMS(uint32_t milliseconds)
{
    vTaskDelay(milliseconds / portTICK_RATE_MS);
}

/* RTOS "heartbeat" LED task */
void vHeartbeatTask(void *pvParameters)
{
    while(1)
    {
        DelayMS(500);
        GPIO_WriteBit(RTOS_LED_PORT,RTOS_LED_PIN, Bit_SET);
        DelayMS(500);
        GPIO_WriteBit(RTOS_LED_PORT,RTOS_LED_PIN, Bit_RESET);
    }
}

/* Read/save the various analog inputs */
void vADCTask(void *pvParameters)
{
    while(1)
    {
        if(gWhereAmI == HomeScreen)
        {
            // RF Amplifier Power Detection (PDET)
            gPDETVoltage = 3.0*((float)gADC3ConvertedValue/(float)0xFFF);

            // Re-draw home screen
            drawHomescreen();
        }
        DelayMS(250);
    }
}

/* User interface task */
void vUITask(void *pvParameters)
{
    static int tickRate = TICK_INITIALRATE;
    static uint32_t holdCount = 0;

    while(1)
    {
        // Some button must be currently down
        if(gPendingButton)
        {
            /* Button is being held down, work out what we want to do
            based on which button is being held */
            if(holdCount == TICK_HOLDCOUNT)
            {
                if((gPendingButton == ButtonUp)||(gPendingButton == ButtonDown))
                    tickRate = TICK_FAST;
                // Special case for SELECT: toggle the RF output
                else if(gPendingButton == ButtonEnter)
                {
                    if(gEnabled)
                    {
                        gEnabled = 0;
                        // Disable synthesizer
                        SynthEnable(false);
                        // Disable amplifier
                        GPIO_WriteBit(AMP_PENABLE_PORT, AMP_PENABLE_PIN, Bit_RESET);
                        splash("RF output DISABLED");
                    }
                    else if(gWhereAmI != DisclaimerScreen)
                    {
                        gEnabled = 1;
                        // Enable synthesizer
                        SynthEnable(true);
                        // Enable amplifier
                        GPIO_WriteBit(AMP_PENABLE_PORT, AMP_PENABLE_PIN, Bit_SET);
                        splash("RF output ENABLED");
                    }
                }
            }

            /* Take care of button de-bouncing and call the button handler
            when the button has been held down long enough */
            if(holdCount % tickRate == 10)
                doMenu(gPendingButton);

            if(holdCount == UINT32_MAX)
                holdCount = 0;
            else
                holdCount++;
        }
        // No buttons held down
        else
        {
            holdCount = 0;
            tickRate = TICK_SLOW;
        }
        DelayMS(5);
    }
}

/* Jamming task */
void vJammingTask(void *pvParameters)
{
    while(1)
    {
        if(gEnabled)
            AdvanceScan(&gScanSettings);
        taskYIELD();
    }
}

/* Main */
int main(void)
{
    /* At this stage the microcontroller clock setting is already configured,
    this is done through SystemInit() function which is called from startup
    file (startup_stm32f4xx.S) before to branch to application main.
    To reconfigure the default setting of SystemInit() function, refer to
    system_stm32f4xx.c file */

    // Initialize the scan settings with defaults
    ScanInit(&gScanSettings);

    /* Setup STM32 hardware */
    prvSetupHardware();

    /* Create FreeRTOS tasks */
    xTaskCreate(vHeartbeatTask,
                (signed portCHAR *)"vHeartbeatTask",
                512,
                NULL,
                tskIDLE_PRIORITY + 2,
                NULL);
    xTaskCreate(vUITask,
                (signed portCHAR *)"vUITask",
                512,
                NULL,
                tskIDLE_PRIORITY + 2,
                NULL);
    xTaskCreate(vJammingTask,
                (signed portCHAR *)"vJammingTask",
                512,
                NULL,
                tskIDLE_PRIORITY + 2,
                NULL);
    xTaskCreate(vADCTask,
                (signed portCHAR *)"vADCTask",
                512,
                NULL,
                tskIDLE_PRIORITY + 2,
                NULL);
    /* Start FreeRTOS */
    vTaskStartScheduler();

    /* Main never returns */
    while(1);
    return 0;
}

/* Initialize necessary hardware */
void prvSetupHardware(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* Configure HCLK clock as SysTick clock source */
    SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK);
    /* GPIO Periph clock enable */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOB | RCC_AHB1Periph_GPIOD | RCC_AHB1Periph_GPIOE, ENABLE);
    /* SYSCFG APB clock must be enabled to get write access to SYSCFG_EXTICRx registers */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

    /* Enable FPU */
    #if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
        SCB->CPACR |= ((3UL << 10*2)|(3UL << 11*2));    // Set CP10 and CP11 Full Access
    #endif

    /* RTOS "heartbeat" LED */
    GPIO_InitStructure.GPIO_Pin     = RTOS_LED_PIN;
    GPIO_InitStructure.GPIO_Mode    = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType   = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed   = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd    = GPIO_PuPd_NOPULL;
    GPIO_Init(RTOS_LED_PORT, &GPIO_InitStructure);
    // Switch off LED
    GPIO_ResetBits(RTOS_LED_PORT, RTOS_LED_PIN);

    /* OLED */
    // RST
    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin     = OLED_RST_PIN;
    GPIO_InitStructure.GPIO_Mode    = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType   = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed   = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd    = GPIO_PuPd_NOPULL;
    GPIO_Init(OLED_RST_PORT, &GPIO_InitStructure);
    // DC
    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin     = OLED_DC_PIN;
    GPIO_InitStructure.GPIO_Mode    = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType   = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed   = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd    = GPIO_PuPd_NOPULL;
    GPIO_Init(OLED_DC_PORT, &GPIO_InitStructure);
    // SPI1
    SPI1_Config();
    // Initialization
    oledInit();
    drawUI(gWhereAmI);

    /* SYNTH */
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
    GPIO_InitStructure.GPIO_Mode    = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_OType   = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed   = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd    = GPIO_PuPd_NOPULL;
    GPIO_Init(SYNTH_MODEGPO6_PORT, &GPIO_InitStructure);
    // Initialize device
    SynthInit();

    /* Joystick */
    SetupJoystick();

    /* RF Amplifier */
    // PENABLE
    // Note: Use external pull-down resistor (1k) on PENABLE to ensure amplifier is OFF at power-on
    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = AMP_PENABLE_PIN;
    GPIO_InitStructure.GPIO_Mode    = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType   = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed   = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd    = GPIO_PuPd_NOPULL;
    GPIO_Init(AMP_PENABLE_PORT, &GPIO_InitStructure);
    // Initially set LOW (amplifier disabled)
    GPIO_ResetBits(AMP_PENABLE_PORT, AMP_PENABLE_PIN);

    /* Variable Gain Amp */
    // PUP
    // Note: Use external pull-down resistor (1k) on PUP to ensure attenuation is at maximum at power-on
    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = VARGAINAMP_PUP_PIN;
    GPIO_InitStructure.GPIO_Mode    = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType   = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed   = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd    = GPIO_PuPd_NOPULL;
    GPIO_Init(VARGAINAMP_PUP_PORT, &GPIO_InitStructure);
    // Initially set LOW (Attenuation at Max, 31.5dB)
    GPIO_ResetBits(VARGAINAMP_PUP_PORT, VARGAINAMP_PUP_PIN);
    // CLK
    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = VARGAINAMP_CLK_PIN;
    GPIO_InitStructure.GPIO_Mode    = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType   = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed   = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd    = GPIO_PuPd_NOPULL;
    GPIO_Init(VARGAINAMP_CLK_PORT, &GPIO_InitStructure);
    // Initially set LOW
    GPIO_ResetBits(VARGAINAMP_CLK_PORT, VARGAINAMP_CLK_PIN);
    // DATA
    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = VARGAINAMP_DATA_PIN;
    GPIO_InitStructure.GPIO_Mode    = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType   = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed   = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd    = GPIO_PuPd_NOPULL;
    GPIO_Init(VARGAINAMP_DATA_PORT, &GPIO_InitStructure);
    // Initially set LOW
    GPIO_ResetBits(VARGAINAMP_DATA_PORT, VARGAINAMP_DATA_PIN);
    // LE
    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = VARGAINAMP_LE_PIN;
    GPIO_InitStructure.GPIO_Mode    = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType   = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed   = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd    = GPIO_PuPd_NOPULL;
    GPIO_Init(VARGAINAMP_LE_PORT, &GPIO_InitStructure);
    // Initially set LOW
    GPIO_ResetBits(VARGAINAMP_LE_PORT, VARGAINAMP_LE_PIN);
    // Set gain to minimum (-13.5dB)
    VarGainAmpSetGain(-13.5);

    /* ADC */
    ADC3_CH12_DMA_Config();
    // Start ADC3 Software Conversion
    ADC_SoftwareStartConv(ADC3);

    /* Nested Vectored Interrupt Controller */
    NVIC_Config();
}

void SetupJoystick(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    // Note: Internal pull-ups are used on each button input (i.e. external pull-ups not required)

    // Configure LEFT as input floating
    GPIO_InitStructure.GPIO_Pin     = JOYSTICK_LEFT_PIN;
    GPIO_InitStructure.GPIO_Mode    = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_OType   = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd    = GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_Speed   = GPIO_Speed_100MHz;
    GPIO_Init(JOYSTICK_LEFT_PORT, &GPIO_InitStructure);
    // Configure RIGHT as input floating
    GPIO_InitStructure.GPIO_Pin     = JOYSTICK_RIGHT_PIN;
    GPIO_InitStructure.GPIO_Mode    = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_OType   = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd    = GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_Speed   = GPIO_Speed_100MHz;
    GPIO_Init(JOYSTICK_RIGHT_PORT, &GPIO_InitStructure);
    // Configure UP as input floating
    GPIO_InitStructure.GPIO_Pin     = JOYSTICK_UP_PIN;
    GPIO_InitStructure.GPIO_Mode    = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_OType   = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd    = GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_Speed   = GPIO_Speed_100MHz;
    GPIO_Init(JOYSTICK_UP_PORT, &GPIO_InitStructure);
    // Configure DOWN as input floating
    GPIO_InitStructure.GPIO_Pin     = JOYSTICK_DOWN_PIN;
    GPIO_InitStructure.GPIO_Mode    = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_OType   = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd    = GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_Speed   = GPIO_Speed_100MHz;
    GPIO_Init(JOYSTICK_DOWN_PORT, &GPIO_InitStructure);
    // Configure SEL as input floating
    GPIO_InitStructure.GPIO_Pin     = JOYSTICK_SELECT_PIN;
    GPIO_InitStructure.GPIO_Mode    = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_OType   = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd    = GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_Speed   = GPIO_Speed_100MHz;
    GPIO_Init(JOYSTICK_SELECT_PORT, &GPIO_InitStructure);

    // LEFT Button
    SYSCFG_EXTILineConfig(JOYSTICK_LEFT_PORT_SOURCE, JOYSTICK_LEFT_PIN_SOURCE);
    // RIGHT Button
    SYSCFG_EXTILineConfig(JOYSTICK_RIGHT_PORT_SOURCE, JOYSTICK_RIGHT_PIN_SOURCE);
    // DOWN Button
    SYSCFG_EXTILineConfig(JOYSTICK_DOWN_PORT_SOURCE, JOYSTICK_DOWN_PIN_SOURCE);
    // UP Button
    SYSCFG_EXTILineConfig(JOYSTICK_UP_PORT_SOURCE, JOYSTICK_UP_PIN_SOURCE);
    // SEL Button
    SYSCFG_EXTILineConfig(JOYSTICK_SELECT_PORT_SOURCE, JOYSTICK_SELECT_PIN_SOURCE);

    // Configure EXTI
    EXTI_InitTypeDef EXTI_InitStructure;
    EXTI_InitStructure.EXTI_Line    = JOYSTICK_LEFT_EXTI_LINE | JOYSTICK_RIGHT_EXTI_LINE | JOYSTICK_UP_EXTI_LINE | JOYSTICK_DOWN_EXTI_LINE | JOYSTICK_SELECT_EXTI_LINE;
    EXTI_InitStructure.EXTI_Mode    = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);
}

void NVIC_Config(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;

    /* Set the Vector Table base address at 0x08000000 */
    NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x00);

    /* Ensure all priority bits are assigned as preemption priority bits. */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

    /* Enable the EXTI15_10 Interrupt */
    NVIC_InitStructure.NVIC_IRQChannel                  = EXTI15_10_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority    = 2;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority       = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd               = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

void SPI1_Config(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure;
    SPI_InitTypeDef   SPI_InitStructure;

    /* Configure SPI1 pins: SCK and MOSI */
    // SCK=PA5, MOSI=PA7
    GPIO_InitStructure.GPIO_Pin     = GPIO_Pin_5 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode    = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType   = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed   = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_PuPd    = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource5, GPIO_AF_SPI1);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource7, GPIO_AF_SPI1);

    /* Configure CS pin */
    GPIO_InitStructure.GPIO_Pin     = OLED_CS_PIN;
    GPIO_InitStructure.GPIO_Mode    = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType   = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed   = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_PuPd    = GPIO_PuPd_UP;
    GPIO_Init(OLED_CS_PORT, &GPIO_InitStructure);

    /* SPI1 Periph clock enable */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);

    /* SPI1 Config */
    // Note: OLED has maximum allowable clock speed = 10 MHz (i.e. minimum Tcycle=100ns)
    SPI_InitStructure.SPI_Direction         = SPI_Direction_1Line_Tx;
    SPI_InitStructure.SPI_Mode              = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize          = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL              = SPI_CPOL_High;
    SPI_InitStructure.SPI_CPHA              = SPI_CPHA_2Edge;               // Read data on RISING edge of clock
    SPI_InitStructure.SPI_NSS               = SPI_NSS_Soft | SPI_NSSInternalSoft_Set;
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16;     // SCK Freq = SystemFrequency/APB2 Prescaler/SPI_BaudRatePrescaler -> 168/2/16 = 5.25 MHz
    SPI_InitStructure.SPI_FirstBit          = SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_CRCPolynomial     = 7;
    SPI_Init(SPI1, &SPI_InitStructure);

    /* SPI1 enable */
    SPI_Cmd(SPI1, ENABLE);
}

void ADC3_CH12_DMA_Config(void)
{
    ADC_InitTypeDef       ADC_InitStructure;
    ADC_CommonInitTypeDef ADC_CommonInitStructure;
    DMA_InitTypeDef       DMA_InitStructure;
    GPIO_InitTypeDef      GPIO_InitStructure;

    /* Enable ADC3, DMA2 and GPIO clocks */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2 | RCC_AHB1Periph_GPIOC, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC3, ENABLE);

    /* DMA2 Stream0 channel0 configuration */
    DMA_InitStructure.DMA_Channel               = DMA_Channel_2;
    DMA_InitStructure.DMA_PeripheralBaseAddr    = (uint32_t)ADC3_DR_ADDRESS;
    DMA_InitStructure.DMA_Memory0BaseAddr       = (uint32_t)&gADC3ConvertedValue;
    DMA_InitStructure.DMA_DIR                   = DMA_DIR_PeripheralToMemory;
    DMA_InitStructure.DMA_BufferSize            = 1;
    DMA_InitStructure.DMA_PeripheralInc         = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc             = DMA_MemoryInc_Disable;
    DMA_InitStructure.DMA_PeripheralDataSize    = DMA_PeripheralDataSize_HalfWord;
    DMA_InitStructure.DMA_MemoryDataSize        = DMA_MemoryDataSize_HalfWord;
    DMA_InitStructure.DMA_Mode                  = DMA_Mode_Circular;
    DMA_InitStructure.DMA_Priority              = DMA_Priority_High;
    DMA_InitStructure.DMA_FIFOMode              = DMA_FIFOMode_Disable;
    DMA_InitStructure.DMA_FIFOThreshold         = DMA_FIFOThreshold_HalfFull;
    DMA_InitStructure.DMA_MemoryBurst           = DMA_MemoryBurst_Single;
    DMA_InitStructure.DMA_PeripheralBurst       = DMA_PeripheralBurst_Single;
    DMA_Init(DMA2_Stream0, &DMA_InitStructure);
    DMA_Cmd(DMA2_Stream0, ENABLE);

    /* Configure ADC3 Channel12 pin as analog input */
    GPIO_InitStructure.GPIO_Pin     = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Mode    = GPIO_Mode_AN;
    GPIO_InitStructure.GPIO_PuPd    = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    /* ADC Common Init */
    ADC_CommonInitStructure.ADC_Mode                = ADC_Mode_Independent;
    ADC_CommonInitStructure.ADC_Prescaler           = ADC_Prescaler_Div2;
    ADC_CommonInitStructure.ADC_DMAAccessMode       = ADC_DMAAccessMode_Disabled;
    ADC_CommonInitStructure.ADC_TwoSamplingDelay    = ADC_TwoSamplingDelay_5Cycles;
    ADC_CommonInit(&ADC_CommonInitStructure);

    /* ADC3 Init */
    ADC_InitStructure.ADC_Resolution            = ADC_Resolution_12b;
    ADC_InitStructure.ADC_ScanConvMode          = DISABLE;
    ADC_InitStructure.ADC_ContinuousConvMode    = ENABLE;
    ADC_InitStructure.ADC_ExternalTrigConvEdge  = ADC_ExternalTrigConvEdge_None;
    ADC_InitStructure.ADC_DataAlign             = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfConversion       = 1;
    ADC_Init(ADC3, &ADC_InitStructure);

    /* ADC3 regular channel12 configuration */
    ADC_RegularChannelConfig(ADC3, ADC_Channel_12, 1, ADC_SampleTime_3Cycles);

    /* Enable DMA request after last transfer (Single-ADC mode) */
    ADC_DMARequestAfterLastTransferCmd(ADC3, ENABLE);

    /* Enable ADC3 DMA */
    ADC_DMACmd(ADC3, ENABLE);

    /* Enable ADC3 */
    ADC_Cmd(ADC3, ENABLE);
}

extern "C"
void EXTI15_10_IRQHandler(void)
{
    /* Joystick LEFT */
    if(EXTI_GetITStatus(EXTI_Line10) != RESET)
    {
        if(!GPIO_ReadInputDataBit(JOYSTICK_LEFT_PORT, JOYSTICK_LEFT_PIN))
            gPendingButton |= ButtonLeft;
        else
            gPendingButton &= ~ButtonLeft;
        EXTI_ClearITPendingBit(EXTI_Line10);
    }
    /* Joystick RIGHT */
    if(EXTI_GetITStatus(EXTI_Line11) != RESET)
    {
        if(!GPIO_ReadInputDataBit(JOYSTICK_RIGHT_PORT, JOYSTICK_RIGHT_PIN))
            gPendingButton |= ButtonRight;
        else
            gPendingButton &= ~ButtonRight;
        EXTI_ClearITPendingBit(EXTI_Line11);
    }
    /* Joystick UP */
    if(EXTI_GetITStatus(EXTI_Line12) != RESET)
    {
        if(!GPIO_ReadInputDataBit(JOYSTICK_UP_PORT, JOYSTICK_UP_PIN))
            gPendingButton = ButtonUp;
        else
            gPendingButton &= ~ButtonUp;
        EXTI_ClearITPendingBit(EXTI_Line12);
    }
    /* Joystick DOWN */
    if(EXTI_GetITStatus(EXTI_Line13) != RESET)
    {
        if(!GPIO_ReadInputDataBit(JOYSTICK_DOWN_PORT, JOYSTICK_DOWN_PIN))
            gPendingButton |= ButtonDown;
        else
            gPendingButton &= ~ButtonDown;
        EXTI_ClearITPendingBit(EXTI_Line13);
    }
    /* Joystick SELECT */
    if(EXTI_GetITStatus(EXTI_Line14) != RESET)
    {
        if(!GPIO_ReadInputDataBit(JOYSTICK_SELECT_PORT, JOYSTICK_SELECT_PIN))
            gPendingButton |= ButtonEnter;
        else
            gPendingButton &= ~ButtonEnter;
        EXTI_ClearITPendingBit(EXTI_Line14);
    }
}

extern "C"
void vApplicationMallocFailedHook( void )
{
    /* vApplicationMallocFailedHook() will only be called if
    configUSE_MALLOC_FAILED_HOOK is set to 1 in FreeRTOSConfig.h.  It is a hook
    function that will get called if a call to pvPortMalloc() fails.
    pvPortMalloc() is called internally by the kernel whenever a task, queue,
    timer or semaphore is created.  It is also called by various parts of the
    demo application.  If heap_1.c or heap_2.c are used, then the size of the
    heap available to pvPortMalloc() is defined by configTOTAL_HEAP_SIZE in
    FreeRTOSConfig.h, and the xPortGetFreeHeapSize() API function can be used
    to query the size of free heap space that remains (although it does not
    provide information on how the remaining heap might be fragmented). */
    taskDISABLE_INTERRUPTS();
    while(1);
}

extern "C"
void vApplicationIdleHook( void )
{
    /* vApplicationIdleHook() will only be called if configUSE_IDLE_HOOK is set
    to 1 in FreeRTOSConfig.h.  It will be called on each iteration of the idle
    task.  It is essential that code added to this hook function never attempts
    to block in any way (for example, call xQueueReceive() with a block time
    specified, or call vTaskDelay()).  If the application makes use of the
    vTaskDelete() API function (as this demo application does) then it is also
    important that vApplicationIdleHook() is permitted to return to its calling
    function, because it is the responsibility of the idle task to clean up
    memory allocated by the kernel to any task that has since been deleted. */
}

extern "C"
void vApplicationStackOverflowHook( xTaskHandle pxTask, signed char *pcTaskName )
{
    ( void ) pcTaskName;
    ( void ) pxTask;

    /* Run time stack overflow checking is performed if
    configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
    function is called if a stack overflow is detected. */
    taskDISABLE_INTERRUPTS();
    while(1);
}
