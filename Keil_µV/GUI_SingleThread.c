/**
  ******************************************************************************
  * @file    GUI_SingleThread.c
  * @brief   Gestion de l'interface graphique (emWin / AppWizard) sous RTX5
  *          pour la carte STM32F746G-DISCO.
  *          Contient :
  *          - le thread principal d'affichage (MainTask)
  *          - le thread de rafraîchissement périodique de l'état (Check_IP)
  *          - la fonction GUI_Refresh_Status() affichant IP, angle et distance
  ******************************************************************************
  */

#include "cmsis_os2.h"
#include "GUI.h"
#include "rl_net.h"
#include "AppWizard.h"
#include "main.h"
#include <stdio.h>

/*----------------------------------------------------------------------------
 *      GUIThread : Thread dédié à la boucle graphique emWin (modèle mono-tâche)
 *---------------------------------------------------------------------------*/
#define GUI_THREAD_STK_SZ    (4096U) /* Taille de pile : 4096 octets (nécessaire pour AppWizard) */

static osThreadId_t GUIThread_tid;                      /* Identifiant du thread GUI */
static uint64_t     GUIThread_stk[GUI_THREAD_STK_SZ/8]; /* Pile alignée sur 8 octets */

/* Attributs du thread graphique (priorité basse pour ne pas bloquer le réseau) */
static const osThreadAttr_t GUIThread_attr = {
  .stack_mem  = &GUIThread_stk[0],
  .stack_size = sizeof(GUIThread_stk),
  .priority   = osPriorityIdle
};

static osThreadId_t CheckIP_Thread_tid;                      /* Identifiant du thread de rafraîchissement */
static uint64_t     CheckIP_Thread_stk[GUI_THREAD_STK_SZ/8]; /* Pile alignée sur 8 octets */

/* Attributs du thread de mise à jour périodique de l'écran */
static const osThreadAttr_t CheckIP_Thread_attr = {
  .stack_mem  = &CheckIP_Thread_stk[0],
  .stack_size = sizeof(CheckIP_Thread_stk),
  .priority   = osPriorityIdle
};

extern void MainTask(void); /* Point d'entrée généré par AppWizard (APPW_MainTask.c) */
void Check_IP(void);        /* Prototypes du thread de rafraîchissement */

/**
  * @brief  Crée et démarre les threads graphiques (MainTask et Check_IP).
  * @retval 0 si succès, -1 en cas d'échec de création d'un thread.
  */
int Init_GUIThread (void) {

  /* Lancement de la tâche principale AppWizard / emWin */
  GUIThread_tid = osThreadNew((void *) MainTask, NULL, &GUIThread_attr);
  if (GUIThread_tid == NULL) {
    return(-1);
  }

  /* Lancement de la tâche périodique demandant le rafraîchissement du bandeau */
  CheckIP_Thread_tid = osThreadNew((void *) Check_IP, NULL, &CheckIP_Thread_attr);
  if (CheckIP_Thread_tid == NULL) {
    return(-1);
  }

  return(0);
}

/**
  * @brief  Met à jour le texte d'état sur l'écran LCD :
  *         format "<IPv4> | <angle> | <distance>" si mesure valide,
  *         ou "<IPv4> | <angle>" sinon.
  * @note   Appelée depuis le contexte de la boucle graphique (AppWizard)
  *         lorsque g_ui_refresh_req est positionné.
  */
void GUI_Refresh_Status(void)
{
	uint8_t ip4_addr[NET_ADDR_IP4_LEN] = {0, 0, 0, 0}; /* Buffer pour l'adresse IPv4 locale */
	WM_HWIN hItem;                                     /* Handle du widget texte emWin */
	char texte[64];                                    /* Chaîne formatée à afficher */

	/* Récupération de l'adresse IPv4 attribuée à l'interface Ethernet 0 */
	netIF_GetOption (NET_IF_CLASS_ETH | 0, netIF_OptionIP4_Address, ip4_addr, sizeof (ip4_addr));

	/* Récupération du widget texte d'état (ID = GUI_ID_USER + 6) sur la fenêtre active */
	hItem = WM_GetDialogItem(WM_GetActiveWindow(), GUI_ID_USER + 6);
	if (hItem != 0)
	{
		if (g_last_distance < 0.0f)
		{
			/* Aucune mesure valide : afficher uniquement l'IP et l'angle */
			sprintf(texte, "%d.%d.%d.%d | %d",
			        ip4_addr[0], ip4_addr[1], ip4_addr[2], ip4_addr[3],
			        g_current_angle);
		}
		else
		{
			/* Mesure valide : afficher la distance avec 1 décimale */
			sprintf(texte, "%d.%d.%d.%d | %d | %.1f",
			        ip4_addr[0], ip4_addr[1], ip4_addr[2], ip4_addr[3],
			        g_current_angle, g_last_distance);
		}
		/* Mise à jour du texte et demande de redessin du widget */
		TEXT_SetText(hItem, texte);
		WM_InvalidateWindow(hItem);
	}
}

/**
  * @brief  Thread de fond demandant un rafraîchissement de l'écran toutes les 500 ms.
  */
void Check_IP(void)
{
	while(1)
	{
		osDelay(500);          /* Attente de 500 ms */
		g_ui_refresh_req = 1;  /* Signale à l'IHM de rafraîchir IP, angle et distance */
	}
}
