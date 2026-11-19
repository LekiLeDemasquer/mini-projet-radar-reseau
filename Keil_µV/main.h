/**
  ******************************************************************************
  * @file    main.h 
  * @brief   Header for main.c and UDP_Socket.c modules (Radar à ultrasons UDP)
  ******************************************************************************
  */
  
#ifndef __MAIN_H
#define __MAIN_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f7xx_hal.h"
#include "rl_net.h"
#include "cmsis_os2.h"
#include "hcsr04.h"   /* Librairie HC-SR04 validée sur banc STM32F103 */

/* Exported constants --------------------------------------------------------*/
/* Port UDP (5000 pour l'application Qt et le cahier des charges) */
#define UDP_PORT_QT                 5000U
#define UDP_PORT_CDC                5000U

/* Constantes du servomoteur DM996 (PWM 50 Hz sur PF6 / TIM10) */
#define SERVO_ANGLE_MIN             0
#define SERVO_ANGLE_MAX             180
#define SERVO_ANGLE_INIT            90
#define SERVO_PULSE_MIN_US          1000U   /* 1.0 ms pour 0°   */
#define SERVO_PULSE_MAX_US          2000U   /* 2.0 ms pour 180° */
#define SERVO_PULSE_INIT_US         1500U   /* 1.5 ms pour 90°  */
#define SERVO_STAB_DELAY_MS         150U    /* Stabilisation du servo + espacement >= 60 ms */

/* Capteur ultrason HC-SR04 : voir hcsr04.h (TRIG PB14, ECHO PI0 - D5, TIM6 à 1 MHz) */
#define THREAD_FLAG_CMD             0x0001U /* Réveil de Thread_Test par udp_cb_func() */

/* Paramètres de balayage SCAN (un seul datagramme UDP) */
#define SCAN_MAX_POINTS             60      /* Limite le nombre de mesures pour tenir dans 1 datagramme */
#define UDP_SCAN_BUF_SIZE           1400U   /* Taille max du datagramme (< MTU 1500 octets) */

/* Exported variables --------------------------------------------------------*/
extern volatile int g_current_angle;
extern volatile float g_last_distance;

extern volatile int g_scan_pending;
extern volatile int g_scan_start;
extern volatile int g_scan_end;
extern volatile int g_scan_step;
extern NET_ADDR g_scan_client_addr;

extern volatile int g_angle_pending;
extern volatile int g_angle_target;
extern NET_ADDR g_angle_client_addr;
extern int32_t g_angle_sock;
extern volatile int g_ui_refresh_req;

extern volatile int g_dist_pending;
extern NET_ADDR g_dist_client_addr;
extern int32_t g_dist_sock;

extern osThreadId_t tid_Test;

/* Exported functions ------------------------------------------------------- */
void servo_set_angle(int deg);
int lire_potentiometre(void);
void GUI_Refresh_Status(void);

#endif /* __MAIN_H */
