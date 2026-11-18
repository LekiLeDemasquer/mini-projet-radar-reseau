/**
  ******************************************************************************
  * @file    hcsr04.c
  * @brief   Driver HC-SR04 (accès direct CMSIS) - algorithme validé sur banc
  *          STM32F103C8T6, porté sur STM32F746NGH sous RTX5.
  *          Base de temps : timer à 1 MHz (1 tick = 1 µs).
  ******************************************************************************
  */

#include "hcsr04.h"

#ifdef _RTE_
#include "RTE_Components.h"
#endif

#ifdef RTE_CMSIS_RTOS2
#include "cmsis_os2.h"
/* Sous RTOS, on bloque l'ordonnanceur pendant la mesure : aucun autre thread
 * (netCore, GUI...) ne peut préempter la boucle d'attente du front descendant
 * et rallonger artificiellement la durée mesurée. Les interruptions restent
 * actives (Ethernet, SysTick, LTDC) : leur durée de quelques µs est négligeable. */
#define HCSR04_LOCK()     int32_t hcsr04_lock_state = osKernelLock()
#define HCSR04_UNLOCK()   (void)osKernelRestoreLock(hcsr04_lock_state)
#else
#define HCSR04_LOCK()     do { } while (0)
#define HCSR04_UNLOCK()   do { } while (0)
#endif

/**
  * @brief  Initialise le driver HC-SR04 et démarre le compteur du Timer.
  */
void HCSR04_Init(void)
{
    /* Mettre la broche TRIG à 0 par défaut */
    HCSR04_TRIG_LOW();

    /* S'assurer que le compteur est remis à 0 */
    HCSR04_TIM->CNT = 0;

    /* Démarrage du compteur du Timer (Bit CEN dans CR1) */
    HCSR04_TIM->CR1 |= TIM_CR1_CEN;
}

/**
  * @brief  Génère l'impulsion TRIG et mesure la largeur de l'impulsion ECHO.
  * @retval Distance en cm (2.0f à 400.0f) ou -1.0f si erreur/timeout.
  */
float HCSR04_ReadDistance(void)
{
    uint32_t duree_us;
    float distance_cm;

    HCSR04_LOCK();

    /* 1. Maintien de TRIG à l'état bas pendant au moins 2 µs pour stabilisation */
    HCSR04_TRIG_LOW();
    HCSR04_TIM->CNT = 0;
    while (HCSR04_TIM->CNT < HCSR04_TRIG_PRE_WAIT_US)
    {
        /* Attente 2 µs */
    }

    /* 2. Génération de l'impulsion TRIG (front haut de 10 µs minimum) */
    HCSR04_TRIG_HIGH();
    HCSR04_TIM->CNT = 0;
    while (HCSR04_TIM->CNT < HCSR04_TRIG_PULSE_US)
    {
        /* Attente 10 µs */
    }
    HCSR04_TRIG_LOW();

    /* 3. Attente du front montant de ECHO avec timeout */
    HCSR04_TIM->CNT = 0;
    while (HCSR04_READ_ECHO() == 0U)
    {
        if (HCSR04_TIM->CNT >= HCSR04_TIMEOUT_US)
        {
            HCSR04_UNLOCK();
            return -1.0f; /* Timeout : le capteur n'a pas répondu */
        }
    }

    /* 4. Mesure de la durée du niveau haut de ECHO */
    HCSR04_TIM->CNT = 0;
    while (HCSR04_READ_ECHO() != 0U)
    {
        if (HCSR04_TIM->CNT >= HCSR04_TIMEOUT_US)
        {
            HCSR04_UNLOCK();
            return -1.0f; /* Timeout : écho trop long ou cible hors de portée */
        }
    }
    duree_us = HCSR04_TIM->CNT;

    HCSR04_UNLOCK();

    /* 5. distance_cm = (duree_us * 0.0343 cm/µs) / 2 (aller-retour) */
    distance_cm = (float)duree_us * HCSR04_SPEED_OF_SOUND_CM / 2.0f;

    /* 6. Validation de la plage nominale (2 cm à 400 cm) */
    if ((distance_cm < HCSR04_MIN_DISTANCE_CM) || (distance_cm > HCSR04_MAX_DISTANCE_CM))
    {
        return -1.0f;
    }

    return distance_cm;
}
