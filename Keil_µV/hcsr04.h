/**
  ******************************************************************************
  * @file    hcsr04.h
  * @brief   Driver pour le capteur de distance à ultrasons HC-SR04.
  *          Version validée sur banc STM32F103C8T6, portée sur STM32F746NGH
  *          (carte STM32F746G-DISCO, projet Keil µVision + RTX5).
  *
  *          Seule la section "CONFIGURATION MATERIELLE" dépend de la cible.
  ******************************************************************************
  */

#ifndef HCSR04_H
#define HCSR04_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f7xx.h"   /* STM32F103 : "stm32f1xx.h" */
#include <stdint.h>

/* ====================================================================
 * CONFIGURATION MATERIELLE (STM32F746NGH - STM32F746G-DISCO)
 * ==================================================================== */

/* --- Broche TRIG : PB14 (connecteur Arduino D12, sortie Push-Pull) --- */
#define HCSR04_TRIG_PORT          GPIOB
#define HCSR04_TRIG_PIN           14U

/* --- Broche ECHO : PI0 (connecteur Arduino D5, entrée tolérante 5V) --- */
#define HCSR04_ECHO_PORT          GPIOI
#define HCSR04_ECHO_PIN           0U

/* --- Timer matériel pour base de temps 1 tick = 1 µs ---
 * TIM6 sur APB1 : HCLK 200 MHz / 4 = 50 MHz -> horloge timer x2 = 100 MHz
 * PSC = 99 -> 1 MHz (configuré dans main.c) */
#define HCSR04_TIM                TIM6

/* ====================================================================
 * CONSTANTES & TIMINGS DU CAPTEUR (identiques au banc STM32F103)
 * ==================================================================== */
#define HCSR04_TRIG_PRE_WAIT_US   2U      /* Attente niveau bas avant impulsion */
#define HCSR04_TRIG_PULSE_US      10U     /* Largeur d'impulsion TRIG (min 10 µs) */
#define HCSR04_TIMEOUT_US         30000U  /* Timeout front montant / descendant (30 ms) */
#define HCSR04_SPEED_OF_SOUND_CM  0.0343f /* Vitesse du son : 343 m/s = 0.0343 cm/µs */
#define HCSR04_MIN_DISTANCE_CM    2.0f    /* Portée minimale HC-SR04 */
#define HCSR04_MAX_DISTANCE_CM    400.0f  /* Portée maximale HC-SR04 */
#define HCSR04_MIN_PERIOD_MS      60U     /* Espacement minimal entre 2 mesures (datasheet) */

/* ====================================================================
 * MACROS D'ACCES REGISTRES CMSIS (portables F1 / F7)
 * Note : le STM32F7 ne possède pas de registre BRR -> on utilise la moitié
 * haute de BSRR pour la remise à zéro (atomique, valable aussi sur F1).
 * ==================================================================== */
#define HCSR04_TRIG_LOW()         (HCSR04_TRIG_PORT->BSRR = (1UL << (HCSR04_TRIG_PIN + 16U)))
#define HCSR04_TRIG_HIGH()        (HCSR04_TRIG_PORT->BSRR = (1UL << HCSR04_TRIG_PIN))
#define HCSR04_READ_ECHO()        ((HCSR04_ECHO_PORT->IDR >> HCSR04_ECHO_PIN) & 0x01U)

/* ====================================================================
 * PROTOTYPES DES FONCTIONS
 * ==================================================================== */

/**
  * @brief  Initialise le driver HC-SR04 (TRIG à 0, démarre le timer).
  *         Les horloges GPIO/TIM et les modes GPIO doivent être configurés avant.
  */
void HCSR04_Init(void);

/**
  * @brief  Génère l'impulsion TRIG et mesure le temps d'aller-retour ECHO.
  * @note   NON réentrante : à n'appeler que depuis UN SEUL thread (Thread_Test).
  * @retval Distance en centimètres (2.0 à 400.0 cm), ou -1.0f si timeout / hors plage.
  */
float HCSR04_ReadDistance(void);

#ifdef __cplusplus
}
#endif

#endif /* HCSR04_H */
