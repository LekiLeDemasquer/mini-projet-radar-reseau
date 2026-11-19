#include "stm32f7xx.h"                  // Device header
#include "main.h"
#include "rl_net.h"
#include "GUI.h"
#include "AppWizard.h"
#include "TEXT.h"
#include <stdio.h>
#include <string.h>

extern void configuration_overclock(void);
extern int Init_GUIThread (void);
extern void Init_Ethernet (void);
extern int32_t udp_sock;

#include "RTE_Components.h"             // Component selection
#include "cmsis_os2.h"                  // ::CMSIS:RTOS2

/*----------------------------------------------------------------------------
 *      Variables globales partagées avec UDP_Socket.c
 *---------------------------------------------------------------------------*/
volatile int g_current_angle = SERVO_ANGLE_INIT;
volatile float g_last_distance = -1.0f;

volatile int g_scan_pending = 0;
volatile int g_scan_start = 0;
volatile int g_scan_end = 180;
volatile int g_scan_step = 15;
NET_ADDR g_scan_client_addr;

volatile int g_angle_pending = 0;
volatile int g_angle_target = SERVO_ANGLE_INIT;
NET_ADDR g_angle_client_addr;
int32_t g_angle_sock = 0;
volatile int g_ui_refresh_req = 0;

/* Commande DIST déléguée à Thread_Test (seul thread autorisé à piloter le HC-SR04) */
volatile int g_dist_pending = 0;
NET_ADDR g_dist_client_addr;
int32_t g_dist_sock = 0;

/*----------------------------------------------------------------------------
 *      Prototypes des fonctions
 *---------------------------------------------------------------------------*/
void servo_set_angle(int deg);
static float mesure_distance(void);
int lire_potentiometre(void);


/*----------------------------------------------------------------------------
 *      Fonction : servo_set_angle(int deg)
 *      Oriente le servomoteur entre 0° et 180° (PWM 50 Hz sur PF6 - TIM10)
 *---------------------------------------------------------------------------*/
void servo_set_angle(int deg)
{
    if (deg < SERVO_ANGLE_MIN)
    {
        deg = SERVO_ANGLE_MIN;
    }
    if (deg > SERVO_ANGLE_MAX)
    {
        deg = SERVO_ANGLE_MAX;
    }

    g_current_angle = deg;
    TIM10->CCR1 = SERVO_PULSE_MIN_US + (deg * (SERVO_PULSE_MAX_US - SERVO_PULSE_MIN_US)) / SERVO_ANGLE_MAX;
}

/*----------------------------------------------------------------------------
 *      Fonction : mesure_distance(void)
 *      Enveloppe de la librairie HC-SR04 (hcsr04.c, validée sur STM32F103) :
 *      - appelée UNIQUEMENT depuis Thread_Test (driver non réentrant, TIM6 partagé)
 *      - garantit un espacement >= 60 ms entre deux salves ultrason (datasheet),
 *        sinon l'écho résiduel de la salve précédente fausse la mesure
 *      - met à jour g_last_distance pour l'écran LCD
 *---------------------------------------------------------------------------*/
static float mesure_distance(void)
{
    static uint32_t derniere_mesure_tick = 0U;
    static int premiere_mesure = 1;
    uint32_t ecart;
    float d_cm;

    if (!premiere_mesure)
    {
        ecart = osKernelGetTickCount() - derniere_mesure_tick; /* OS_TICK_FREQ = 1000 -> 1 tick = 1 ms */
        if (ecart < HCSR04_MIN_PERIOD_MS)
        {
            osDelay(HCSR04_MIN_PERIOD_MS - ecart);
        }
    }

    d_cm = HCSR04_ReadDistance();

    derniere_mesure_tick = osKernelGetTickCount();
    premiere_mesure = 0;
    g_last_distance = d_cm;
    return d_cm;
}

/*----------------------------------------------------------------------------
 *      Fonction : lire_potentiometre(void)
 *---------------------------------------------------------------------------*/
int lire_potentiometre(void)
{
    ADC3->CR2 |= (1U << 30);          /* Démarrer la conversion */
    while (!(ADC3->SR & (1U << 1)))   /* Attendre la fin de conversion */
    {
    }
    return (int)ADC3->DR;             /* Récupérer la valeur */
}

/*----------------------------------------------------------------------------
 *      Tâche Thread_Test : répétée toutes les 100 ms (Cahier des charges §4)
 *      - Exécute le balayage SCAN en dehors de udp_cb_func() pour ne jamais bloquer
 *      - Mesure la distance et met à jour l'affichage (angle et distance)
 *---------------------------------------------------------------------------*/
osThreadId_t tid_Test;

static void Thread_Test (void *argument)
{
    char scan_buffer[UDP_SCAN_BUF_SIZE];
    char ligne[48];
    uint8_t *sendbuf;
    int offset;
    int len_ligne;
    int angle;
    float d_cm;

    (void)argument;

    while (1)
    {
        /* Si une commande DIST a été demandée via UDP (déléguée par udp_cb_func) */
        if (g_dist_pending)
        {
            char reponse[32];
            int len_rep;

            d_cm = mesure_distance();

            if (d_cm < 0.0f)
            {
                len_rep = snprintf(reponse, sizeof(reponse), "DIST TIMEOUT\n");
            }
            else
            {
                len_rep = snprintf(reponse, sizeof(reponse), "DIST %.1f\n", d_cm);
            }

            if ((g_dist_sock > 0) && (len_rep > 0))
            {
                sendbuf = netUDP_GetBuffer((uint32_t)len_rep);
                if (sendbuf != NULL)
                {
                    memcpy(sendbuf, reponse, (size_t)len_rep);
                    netUDP_Send(g_dist_sock, &g_dist_client_addr, sendbuf, (uint32_t)len_rep);
                }
            }

            g_dist_pending = 0;
        }
        /* Si une commande ANGLE a été demandée via UDP */
        else if (g_angle_pending)
        {
            int wait_ui_ms = 0;

            /* 1. Appliquer le nouveau rapport cyclique PWM sur PF6 (TIM10) */
            servo_set_angle(g_angle_target);

            /* 2. Demander la mise à jour immédiate de l'écran LCD */
            g_ui_refresh_req = 1;

            /* 3. Laisser au moins une période PWM (20 ms) + rafraîchissement écran */
            osDelay(25);
            while (g_ui_refresh_req && (wait_ui_ms < 50))
            {
                osDelay(5);
                wait_ui_ms += 5;
            }

            /* 4. Envoyer OK au PC uniquement après mise à jour PWM + écran */
            if (g_angle_sock > 0)
            {
                sendbuf = netUDP_GetBuffer(3U);
                if (sendbuf != NULL)
                {
                    memcpy(sendbuf, "OK\n", 3U);
                    netUDP_Send(g_angle_sock, &g_angle_client_addr, sendbuf, 3U);
                }
            }

            g_angle_pending = 0;
        }
        /* Si une commande SCAN a été demandée via UDP */
        else if (g_scan_pending)
        {
            offset = 0;
            scan_buffer[0] = '\0';

            for (angle = g_scan_start; angle <= g_scan_end; angle += g_scan_step)
            {
                /* Orienter le servomoteur et laisser stabiliser + espacer >= 60 ms */
                servo_set_angle(angle);
                osDelay(SERVO_STAB_DELAY_MS);

                /* Mesurer la distance à cet angle (librairie HC-SR04) */
                d_cm = mesure_distance();

                /* Formater une ligne "<angle> <cm>\n" avec 1 décimale (ou TIMEOUT) */
                if (d_cm < 0.0f)
                {
                    len_ligne = snprintf(ligne, sizeof(ligne), "%d TIMEOUT\n", angle);
                }
                else
                {
                    len_ligne = snprintf(ligne, sizeof(ligne), "%d %.1f\n", angle, d_cm);
                }

                /* Ajouter dans le buffer unique du datagramme en gardant la place pour "END\n" */
                if ((len_ligne > 0) && ((offset + len_ligne + 5) < (int)sizeof(scan_buffer)))
                {
                    memcpy(&scan_buffer[offset], ligne, (size_t)len_ligne);
                    offset += len_ligne;
                    scan_buffer[offset] = '\0';
                }
            }

            /* Terminer le datagramme par "END\n" comme exigé par le cahier des charges */
            len_ligne = snprintf(ligne, sizeof(ligne), "END\n");
            if ((len_ligne > 0) && ((offset + len_ligne) < (int)sizeof(scan_buffer)))
            {
                memcpy(&scan_buffer[offset], ligne, (size_t)len_ligne);
                offset += len_ligne;
                scan_buffer[offset] = '\0';
            }

            /* Envoyer l'unique datagramme UDP contenant tout le balayage vers l'expéditeur */
            if ((udp_sock > 0) && (offset > 0))
            {
                sendbuf = netUDP_GetBuffer((uint32_t)offset);
                if (sendbuf != NULL)
                {
                    memcpy(sendbuf, scan_buffer, (size_t)offset);
                    netUDP_Send(udp_sock, &g_scan_client_addr, sendbuf, (uint32_t)offset);
                }
            }

            g_scan_pending = 0;
        }
        else
        {
            /* Mesure périodique (toutes les 100 ms, > 60 ms requis) pour l'écran LCD.
             * osThreadFlagsWait remplace osDelay(100) : une commande UDP réveille
             * immédiatement le thread (THREAD_FLAG_CMD) au lieu d'attendre 100 ms. */
            (void)mesure_distance();
            (void)osThreadFlagsWait(THREAD_FLAG_CMD, osFlagsWaitAny, 100U);
        }
    }
}

static int Init_TestThread (void)
{
    tid_Test = osThreadNew(Thread_Test, NULL, NULL);
    if (tid_Test == NULL)
    {
        return -1;
    }
    return 0;
}

/*----------------------------------------------------------------------------
 *      Programme principal : Initialisation des périphériques (sans librairie)
 *---------------------------------------------------------------------------*/
int main(void)
{
    configuration_overclock(); // Pour pouvoir utiliser l'écran LCD (SYSCLK = 200 MHz)

    /**********************************
     Début Initialisation périphériques
    **********************************/
    // Entrainement Led (PI1)
    RCC->AHB1ENR |= (1U << 8);
    GPIOI->MODER |= (1U << 2);

    // Entrainement slider (DAC sur PA4)
    RCC->APB1ENR |= (1U << 29); // Activation DAC
    RCC->AHB1ENR |= (1U << 0);  // Activation GPIOA
    GPIOA->MODER |= (3U << 8);  // PA4 analogique
    DAC->CR |= (1U << 0);       // Active sortie PA4

    // Entrainement rotary (desactive : PF6 / TIM10 est utilise pour le servomoteur)
    /*
    RCC->AHB1ENR |= (1U << 5);  // Activation GPIOF
    RCC->APB2ENR |= (1U << 17); // Activation TIM10
    GPIOF->MODER |= (2U << 12); // PF6 en fonction alternative AF3
    GPIOF->AFR[0] |= (3U << 24);
    TIM10->CCMR1 |= (6U << 4);  // Mode PWM 1
    TIM10->CCER |= (1U << 0);   // Activation sortie CH1
    TIM10->PSC = 7;
    TIM10->ARR = 39999;
    TIM10->CCR1 = 4000;         // Rapport cyclique initial 10%
    TIM10->CR1 |= (1U << 0);    // Démarrage TIM10
    */

    // Configuration Servomoteur DM996 sur PF6 - TIM10 (PWM 50 Hz, pas de 1 µs)
    // Horloge APB2 Timer = 200 MHz -> PSC = 199 donne 200 MHz / 200 = 1 MHz (1 tick = 1 µs)
    // ARR = 19999 donne une période de 20 000 µs = 20 ms (50 Hz)
    RCC->AHB1ENR |= (1U << 5);   // Activation GPIOF
    RCC->APB2ENR |= (1U << 17);  // Activation TIM10
    GPIOF->MODER &= ~(3U << 12);
    GPIOF->MODER |= (2U << 12);  // PF6 en fonction alternative
    GPIOF->AFR[0] &= ~(15U << 24);
    GPIOF->AFR[0] |= (3U << 24); // AF3 : TIM10_CH1
    TIM10->CCMR1 &= ~(7U << 4);
    TIM10->CCMR1 |= (6U << 4);   // Mode PWM 1
    TIM10->CCER |= (1U << 0);    // Activation CH1
    TIM10->PSC = 199;            // 1 tick = 1 µs (200 MHz / (199 + 1))
    TIM10->ARR = 19999;          // Période 20 ms (50 Hz)
    TIM10->CCR1 = SERVO_PULSE_INIT_US; // Position initiale 90° (1500 µs)
    TIM10->EGR = (1U << 0);            // Charger immédiatement PSC et ARR
    TIM10->CR1 |= (1U << 0);           // Démarrage TIM10

    // Configuration Capteur Ultrason HC-SR04 (broches/timer utilisés par hcsr04.c) :
    // - TRIG sur PB14 en sortie (Output Push-Pull)
    // - ECHO sur PI0 en entrée (Input, connecteur Arduino D5, tolérant 5V)
    // - Timer TIM6 cadencé à 1 MHz (1 tick = 1 µs) pour l'impulsion TRIG et la mesure de ECHO
    RCC->AHB1ENR |= (1U << 1);   // Activation horloge GPIOB (TRIG sur PB14)
    RCC->AHB1ENR |= (1U << 8);   // Activation horloge GPIOI (ECHO sur PI0 - broche D5)
    RCC->APB1ENR |= (1U << 4);   // Activation horloge TIM6

    // PB14 en sortie digitale (MODER14 = 01), mise à l'état bas par défaut
    GPIOB->MODER &= ~(3U << 28);
    GPIOB->MODER |= (1U << 28);
    GPIOB->OTYPER &= ~(1U << 14);
    GPIOB->OSPEEDR |= (3U << 28);
    GPIOB->PUPDR &= ~(3U << 28);
    GPIOB->ODR &= ~(1U << 14);

    // PI0 (Arduino D5) en entrée digitale (MODER0 = 00) avec résistance de Pull-Down (PUPDR0 = 10)
    GPIOI->MODER &= ~(3U << 0);
    GPIOI->PUPDR &= ~(3U << 0);
    GPIOI->PUPDR |= (2U << 0);

    // Configuration de TIM6 en compteur libre de 1 µs (100 MHz / (99 + 1) = 1 MHz)
    TIM6->CR1 = 0;
    TIM6->PSC = 99;              // 1 tick = 1 µs
    TIM6->ARR = 0xFFFF;          // Comptage jusqu'à 65535 µs (> 30 ms de timeout)
    TIM6->EGR = (1U << 0);       // Générer un événement Update (UG) pour charger le prescaler
    TIM6->CR1 |= (1U << 0);      // Démarrer le compteur TIM6 (CEN = 1)

    HCSR04_Init();               // Librairie HC-SR04 : TRIG à 0 (BSRR atomique), compteur TIM6 à 0

    // Entrainement potentiomètre (PF10 - ADC3)
    RCC->AHB1ENR |= (1U << 5);   // Activation GPIOF
    RCC->APB2ENR |= (1U << 10);  // Activation ADC3
    GPIOF->MODER |= (3U << 20);  // PF10 en analogique
    ADC3->SQR1 &= ~(15U << 20);  // Une seule conversion
    ADC3->SQR3 &= ~(31U << 0);
    ADC3->SQR3 |= (8U << 0);     // Canal 8
    ADC3->CR2 |= (1U << 0);      // Activation ADC3

    /**********************************
     Fin Initialisation périphériques
    **********************************/

    /**********************************
     Utilisation RTOS (Keil RTX5)
    **********************************/
    /* Initialize CMSIS-RTOS2 */
    osKernelInitialize ();

    /* Create thread functions that start executing */
    Init_Ethernet();
    Init_GUIThread();
    Init_TestThread();   // Tâche de test et de balayage (boucle avec osDelay)

    /* Start thread execution */
    osKernelStart();

    /* Infinite loop (ne s'exécute pas avec le RTOS !) */
    while (1)
    {
    }
}
