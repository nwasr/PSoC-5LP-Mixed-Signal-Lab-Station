#include <project.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"

/* ---------- scope config ---------- */
#define FRAME_SAMPLES      252u
#define ADC_TO_8BIT_SHIFT  4u
#define DECIM_FACTOR       2u

/* ---------- waveform generator ---------- */
#define LUT_SIZE           64u
#define WAVE_CLK_HZ        1000000u

/* ---------- R measurement ---------- */
#define IDAC_R_CODE        (50u)
/* effective IDAC current for this code (in Amps) */
#define IDAC_R_CURRENT_A   (0.000414f)  /* ~414 µA, used for Rraw */

/* calibration gain: maps Rraw to real ohms.
 * from your logs: 3.3k -> Rraw ~50, 10k -> ~150 ⇒ factor ≈ 66
 */
#define R_CAL_GAIN         (68.0f)

/* ---------- C measurement ---------- */
/* We measure in the small-signal region 2 mV .. 10 mV,
 * because the node never rises above ~15 mV due to internal loading.
 */
#define IDAC_C_CODE        (12u)
#define IDAC_C_LSB_A       (8e-6f)
#define IDAC_C_CURRENT_A   (IDAC_C_CODE * IDAC_C_LSB_A)

/* new thresholds in the reachable range */
#define DISCHARGE_MS       (50u)
#define SAMPLE_US          (10u)
#define VSTART_MV          (2)
#define VEND_MV            (10)
#define TIMEOUT_MS         (500u)


#define C_CAL_GAIN         (0.014f)   /* start with ~0.11, tweak if needed */

/* ---------- scope buffer ---------- */
static volatile uint8  sampleBuffer[FRAME_SAMPLES];
static volatile uint16 sampleIndex = 0u;
static volatile uint8  frameReady  = 0u;

/* ---------- waveform LUTs ---------- */
static uint8 sineBase[LUT_SIZE];
static uint8 triBase[LUT_SIZE];
static uint8 sqrBase[LUT_SIZE];
static uint8 waveLUT[LUT_SIZE];

static volatile uint16 waveIndex    = 0u;
static volatile uint8  waveMode     = 0u;   /* 0=SINE,1=TRI,2=SQR */
static volatile uint8  amp_percent  = 100u;
static volatile uint8  wave_enabled = 0u;

/* ---------- measurement requests ---------- */
static volatile uint8 meas_r_request = 0u;
static volatile uint8 meas_c_request = 0u;

/* =========================================================
 *  fast sin approximation
 * =======================================================*/
static float my_sin_approx(float x)
{
    while (x > 3.1415926f) x -= 6.2831852f;
    while (x < -3.1415926f) x += 6.2831852f;
    float x2 = x * x;
    float x3 = x * x2;
    float x5 = x3 * x2;
    return x - x3 / 6.0f + x5 / 120.0f;
}

/* =========================================================
 *  build waveform LUTs
 * =======================================================*/
static void build_sine(void)
{
    uint16 i;
    for (i = 0u; i < LUT_SIZE; i++)
    {
        float t = 6.2831852f * (float)i / (float)LUT_SIZE;
        float s = my_sin_approx(t);
        int16 v = (int16)(128.0f + 127.0f * s);
        if (v < 0)   v = 0;
        if (v > 255) v = 255;
        sineBase[i] = (uint8)v;
    }
}

static void build_tri(void)
{
    uint16 h = LUT_SIZE / 2u;
    uint16 i;

    for (i = 0u; i < h; i++)
        triBase[i] = (uint8)((255u * i) / (h - 1u));

    for (i = 0u; i < h; i++)
        triBase[h + i] = (uint8)(255u - (255u * i) / (h - 1u));
}

static void build_sqr(void)
{
    uint16 h = LUT_SIZE / 2u;
    uint16 i;

    for (i = 0u; i < h; i++)
        sqrBase[i] = 0u;
    for (i = h; i < LUT_SIZE; i++)
        sqrBase[i] = 255u;
}

static void rebuild_lut(void)
{
    uint16 i;
    const uint8 *src = (waveMode == 1u) ? triBase :
                       (waveMode == 2u) ? sqrBase : sineBase;

    if (amp_percent > 100u) amp_percent = 100u;

    for (i = 0u; i < LUT_SIZE; i++)
    {
        uint16 s = ((uint16)src[i] * amp_percent) / 100u;
        if (s > 255u) s = 255u;
        waveLUT[i] = (uint8)s;
    }
}

static void set_amplitude(uint8 a)
{
    amp_percent = a;
    if (amp_percent > 100u) amp_percent = 100u;
    rebuild_lut();
}

static void set_wave(uint8 m)
{
    waveMode = m;
    rebuild_lut();
}

static void set_frequency(uint32 f)
{
    if (f < 1u)    f = 1u;
    if (f > 3000u) f = 3000u;

    uint32 rate = f * LUT_SIZE;
    if (rate == 0u) rate = 1u;

    uint32 p = WAVE_CLK_HZ / rate;
    if (p == 0u) p = 1u;
    if (p > 65535u) p = 65535u;
    p--;

    WaveTimer_Stop();
    WaveTimer_WriteCounter(0u);
    WaveTimer_WritePeriod((uint16)p);
    WaveTimer_Start();
}

/* =========================================================
 *  ADC_SAR_1 ISR: oscilloscope sampling
 * =======================================================*/
CY_ISR(ADC_ISR_Handler)
{
    static uint8 dc = 0u;
    uint16 raw = ADC_SAR_1_GetResult16();

    dc++;
    if (dc < DECIM_FACTOR)
        return;
    dc = 0u;

    if (!frameReady)
    {
        sampleBuffer[sampleIndex++] = (uint8)(raw >> ADC_TO_8BIT_SHIFT);
        if (sampleIndex >= FRAME_SAMPLES)
        {
            frameReady  = 1u;
            sampleIndex = 0u;
        }
    }
}

/* =========================================================
 *  WaveTimer ISR: function generator stepping
 * =======================================================*/
CY_ISR(WaveTimer_ISR)
{
    (void)WaveTimer_ReadStatusRegister();

    if (!wave_enabled)
    {
        VDAC8_1_SetValue(0u);
        return;
    }

    VDAC8_1_SetValue(waveLUT[waveIndex++]);
    if (waveIndex >= LUT_SIZE)
        waveIndex = 0u;
}

/* =========================================================
 *  R / C measurement helpers (ADC_SAR_2 + AMux_1 + IDAC_1)
 * =======================================================*/

/* free-running ADC helper for C measurement */
static uint32 wait_until_mv(int target_mv, uint32 timeout_ms)
{
    uint32 elapsed_us = 0u;
    uint32 timeout_us = timeout_ms * 1000u;

    while (elapsed_us < timeout_us)
    {
        int32 mv = ADC_SAR_2_CountsTo_mVolts(ADC_SAR_2_GetResult16());
        if (mv >= target_mv)
            return elapsed_us;

        CyDelayUs(SAMPLE_US);
        elapsed_us += SAMPLE_US;
    }
    return (uint32)-1;
}

/* Resistance on Pin_R to GND */
static int32 Measure_R_Pin(void)
{
    int32 adc_mV;
    float v_pin, r_raw, r_cal;

    /* select Pin_R channel on mux */
    AMux_1_FastSelect(0u);

    Pin_R_SetDriveMode(Pin_R_DM_ALG_HIZ);
    IDAC_1_SetPolarity(IDAC_1_SOURCE);
    IDAC_1_SetValue(IDAC_R_CODE);

    CyDelay(20u);

    /* one fresh sample from free-running ADC_SAR_2 */
    ADC_SAR_2_IsEndConversion(ADC_SAR_2_WAIT_FOR_RESULT);
    adc_mV = ADC_SAR_2_CountsTo_mVolts(ADC_SAR_2_GetResult16());

    v_pin = adc_mV / 1000.0f;
    r_raw = v_pin / IDAC_R_CURRENT_A;    /* uncalibrated */
    if (r_raw < 0.0f) r_raw = 0.0f;

    r_cal = r_raw * R_CAL_GAIN;

    {
        char dbg[80];
        sprintf(dbg, "DBG_R: mv=%ld, Rraw=%ld, Rcal=%ld\r\n",
                (long)adc_mV, (long)r_raw, (long)r_cal);
        UART_PutString(dbg);
    }

    return (int32)r_cal;
}

/* Capacitance on Pin_C to GND (uF), in small 2..10 mV region */
static float Measure_C_Pin_uF(void)
{
    /* select Pin_C channel */
    AMux_1_FastSelect(1u);

    /* fully discharge capacitor */
    IDAC_1_SetValue(0u);
    Pin_C_SetDriveMode(Pin_C_DM_STRONG);
    Pin_C_Write(0u);
    CyDelay(DISCHARGE_MS);

    /* high-Z and small delay */
    Pin_C_SetDriveMode(Pin_C_DM_ALG_HIZ);
    CyDelayUs(50u);

    /* start charging with IDAC */
    IDAC_1_SetPolarity(IDAC_1_SOURCE);
    IDAC_1_SetValue(IDAC_C_CODE);

    /* ADC_SAR_2 is already in free-running mode */
    uint32 t1 = wait_until_mv(VSTART_MV, TIMEOUT_MS);
    uint32 t2 = wait_until_mv(VEND_MV, TIMEOUT_MS);

    IDAC_1_SetValue(0u);  /* stop charging */

    if (t1 == (uint32)-1 || t2 == (uint32)-1 || t2 <= t1)
    {
        UART_PutString("DBG_C: timeout or bad dt\r\n");
        return -1.0f;
    }

    uint32 dt_us = t2 - t1;

    float dV  = (float)(VEND_MV - VSTART_MV) / 1000.0f; /* volts */
    float t_s = dt_us * 1e-6f;
    float C   = (IDAC_C_CURRENT_A * t_s) / dV;          /* Farads */
    C        *= C_CAL_GAIN;                             /* calibration fudge */
    float C_uF = C * 1e6f;

    {
        char dbg[80];
        sprintf(dbg, "DBG_C: dt=%lu us, C=%.3f uF\r\n",
                (unsigned long)dt_us, (double)C_uF);
        UART_PutString(dbg);
    }

    return C_uF;
}

/* =========================================================
 *  UART command parser
 * =======================================================*/
#define CMD_BUF_LEN 64
static char   cmdBuf[CMD_BUF_LEN];
static uint16 cmdLen = 0u;

static void process_cmd(char *cmd)
{
    char *t = strtok(cmd, ",");
    while (t)
    {
        if (!strncmp(t, "FREQ:", 5))
        {
            uint32 f = (uint32)atoi(t + 5);
            set_frequency(f);
        }
        else if (!strncmp(t, "AMP:", 4))
        {
            int a = atoi(t + 4);
            if (a < 0)   a = 0;
            if (a > 100) a = 100;
            set_amplitude((uint8)a);
        }
        else if (!strncmp(t, "WAVE:", 5))
        {
            char *w = t + 5;
            if      (!strcmp(w, "SINE")) set_wave(0u);
            else if (!strcmp(w, "TRI"))  set_wave(1u);
            else if (!strcmp(w, "SQR"))  set_wave(2u);
        }
        else if (!strncmp(t, "EN:", 3))
        {
            int e = atoi(t + 3);
            if (e)
                wave_enabled = 1u;
            else
            {
                wave_enabled = 0u;
                VDAC8_1_SetValue(0u);
                waveIndex = 0u;
            }
        }
        else if (!strncmp(t, "MEAS:", 5))
        {
            char *m = t + 5;
            if (!strcmp(m, "R"))
                meas_r_request = 1u;
            else if (!strcmp(m, "C"))
                meas_c_request = 1u;
        }

        t = strtok(NULL, ",");
    }
}

static void poll_uart_commands(void)
{
    while (UART_GetRxBufferSize() > 0u)
    {
        char c = (char)UART_GetChar();

        if (c == '\r' || c == '\n')
        {
            if (cmdLen > 0u)
            {
                cmdBuf[cmdLen] = '\0';
                process_cmd(cmdBuf);
                cmdLen = 0u;
            }
        }
        else
        {
            if (cmdLen < CMD_BUF_LEN - 1u)
                cmdBuf[cmdLen++] = c;
        }
    }
}

/* =========================================================
 *  main FreeRTOS app task
 * =======================================================*/
static void app_task(void *arg)
{
    (void)arg;

    for (;;)
    {
        poll_uart_commands();

        if (meas_r_request)
        {
            meas_r_request = 0u;
            int32 r = Measure_R_Pin();
            char msg[32];
            sprintf(msg, "R_GND:%ld\r\n", (long)r);
            UART_PutString(msg);
        }

        if (meas_c_request)
        {
            meas_c_request = 0u;
            float c = Measure_C_Pin_uF();
            char msg[40];
            sprintf(msg, "C_uF:%.3f\r\n", (double)c);
            UART_PutString(msg);
        }

        if (frameReady)
        {
            uint8 header[2];
            header[0] = 0xAA;
            header[1] = (uint8)FRAME_SAMPLES;
            UART_PutArray(header, 2);
            UART_PutArray((uint8 *)sampleBuffer, FRAME_SAMPLES);
            frameReady = 0u;
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

/* =========================================================
 *  main
 * =======================================================*/
extern void FreeRTOS_Start(void);

int main(void)
{
    CyGlobalIntEnable;

    UART_Start();

    /* oscilloscope ADC */
    ADC_SAR_1_Start();
    ADC_SAR_1_StartConvert();
    isr_adc_StartEx(ADC_ISR_Handler);

    /* R/C measurement ADC + IDAC + Mux */
    ADC_SAR_2_Start();
    ADC_SAR_2_StartConvert();   /* free-running for R & C */
    IDAC_1_Start();
    AMux_1_Start();

    /* waveform generator */
    build_sine();
    build_tri();
    build_sqr();
    rebuild_lut();

    VDAC8_1_Start();
    VDAC8_1_SetValue(0u);
    WaveClock_Start();
    WaveTimer_Start();
    isr_wave_StartEx(WaveTimer_ISR);
    set_frequency(1000u);

    FreeRTOS_Start();
    xTaskCreate(app_task, "APP", 256u, NULL, 3u, NULL);

    UART_PutString("READY\r\n");

    vTaskStartScheduler();

    for (;;)
    {
        /* should never get here */
    }
}
