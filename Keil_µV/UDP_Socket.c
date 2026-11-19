/*------------------------------------------------------------------------------
 * MDK Middleware - Component ::Network:Service
 * Copyright (c) 2004-2019 Arm Limited (or its affiliates). All rights reserved.
 *------------------------------------------------------------------------------
 * Name:    UDP_Socket.c
 * Purpose: Serveur UDP du Radar à ultrasons (ANGLE, DIST, SCAN)
 *----------------------------------------------------------------------------*/
//! [code_UDP_Socket]
#include "rl_net.h"
#include "stm32f7xx.h"                  // Device header
#include "main.h"
#include "AppWizard.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int32_t udp_sock;                       // Socket UDP sur le port 5000 (Cahier des charges & Interface Qt)

/*----------------------------------------------------------------------------
 *      Fonction utilitaire : Envoie une chaîne ASCII en UDP à l'expéditeur
 *---------------------------------------------------------------------------*/
static void udp_send_response(int32_t socket, const NET_ADDR *addr, const char *msg)
{
    uint32_t msg_len;
    uint8_t *sendbuf;

    if ((socket <= 0) || (addr == NULL) || (msg == NULL))
    {
        return;
    }

    msg_len = (uint32_t)strlen(msg);
    if (msg_len == 0U)
    {
        return;
    }

    sendbuf = netUDP_GetBuffer(msg_len);
    if (sendbuf != NULL)
    {
        memcpy(sendbuf, msg, msg_len);
        netUDP_Send(socket, addr, sendbuf, msg_len);
    }
}

/*----------------------------------------------------------------------------
 *      Callback UDP : appelé à la réception d'un datagramme UDP
 *      Commandes supportées (Cahier des charges §3) :
 *      - ANGLE <a>            -> "OK\n" ou "ERR <motif>\n"
 *      - DIST                 -> "DIST <cm>\n" (1 décimale) ou "DIST TIMEOUT\n"
 *      - SCAN <d> <f> <pas>   -> déclenche le balayage dans Thread_Test()
 *---------------------------------------------------------------------------*/
uint32_t udp_cb_func (int32_t socket, const NET_ADDR *addr, const uint8_t *buf, uint32_t len)
{
    char cmd[128];
    uint32_t copy_len;
    int angle;
    int debut, fin, pas, nb_points;
    char extra;

    if ((buf == NULL) || (len == 0U))
    {
        return 0;
    }

    /* Copier le message reçu dans une chaîne C terminée par '\0' */
    copy_len = (len < (sizeof(cmd) - 1U)) ? len : (sizeof(cmd) - 1U);
    memcpy(cmd, buf, copy_len);
    cmd[copy_len] = '\0';

    /* Supprimer les retours à la ligne (\r, \n) et espaces de fin */
    while ((copy_len > 0U) &&
           ((cmd[copy_len - 1U] == '\n') ||
            (cmd[copy_len - 1U] == '\r') ||
            (cmd[copy_len - 1U] == ' ')  ||
            (cmd[copy_len - 1U] == '\t')))
    {
        cmd[copy_len - 1U] = '\0';
        copy_len--;
    }

    if (copy_len == 0U)
    {
        udp_send_response(socket, addr, "ERR commande vide\n");
        return 0;
    }

    /* 1. Commande : DIST
     * La mesure n'est PLUS faite ici : udp_cb_func() s'exécute dans le thread
     * netCore, qui entrait en concurrence avec Thread_Test sur TIM6/TRIG.
     * On délègue à Thread_Test, seul propriétaire du capteur, qui répondra
     * "DIST <cm>\n" ou "DIST TIMEOUT\n". */
    if (strcmp(cmd, "DIST") == 0)
    {
        if (g_dist_pending)
        {
            udp_send_response(socket, addr, "ERR mesure DIST deja en cours\n");
            return 0;
        }
        g_dist_client_addr = *addr;
        g_dist_sock = socket;
        g_dist_pending = 1;
        (void)osThreadFlagsSet(tid_Test, THREAD_FLAG_CMD);
        return 0;
    }

    /* 2. Commande : ANGLE <a> */
    if (strncmp(cmd, "ANGLE", 5) == 0)
    {
        if (sscanf(cmd, "ANGLE %d %c", &angle, &extra) != 1)
        {
            udp_send_response(socket, addr, "ERR format ANGLE\n");
            return 0;
        }
        if ((angle < SERVO_ANGLE_MIN) || (angle > SERVO_ANGLE_MAX))
        {
            udp_send_response(socket, addr, "ERR angle hors plage (0..180)\n");
            return 0;
        }

        if (g_angle_pending)
        {
            udp_send_response(socket, addr, "ERR commande ANGLE deja en cours\n");
            return 0;
        }

        /* Déléguer à Thread_Test() pour actualiser PWM + écran avant d'envoyer OK */
        g_angle_target = angle;
        g_angle_client_addr = *addr;
        g_angle_sock = socket;
        g_angle_pending = 1;
        (void)osThreadFlagsSet(tid_Test, THREAD_FLAG_CMD);
        return 0;
    }

    /* 3. Commande : SCAN <debut> <fin> <pas> */
    if (strncmp(cmd, "SCAN", 4) == 0)
    {
        if (sscanf(cmd, "SCAN %d %d %d %c", &debut, &fin, &pas, &extra) != 3)
        {
            udp_send_response(socket, addr, "ERR format SCAN\n");
            return 0;
        }
        if ((debut < SERVO_ANGLE_MIN) || (debut > SERVO_ANGLE_MAX) ||
            (fin < SERVO_ANGLE_MIN)   || (fin > SERVO_ANGLE_MAX)   ||
            (debut > fin)             || (pas <= 0))
        {
            udp_send_response(socket, addr, "ERR parametres SCAN hors plage\n");
            return 0;
        }

        nb_points = ((fin - debut) / pas) + 1;
        if (nb_points > SCAN_MAX_POINTS)
        {
            udp_send_response(socket, addr, "ERR trop de mesures pour un datagramme\n");
            return 0;
        }

        if (g_scan_pending)
        {
            udp_send_response(socket, addr, "ERR balayage deja en cours\n");
            return 0;
        }

        /* Déléguer le balayage à Thread_Test() pour ne pas bloquer udp_cb_func() */
        g_scan_start = debut;
        g_scan_end   = fin;
        g_scan_step  = pas;
        g_scan_client_addr = *addr;
        udp_sock = socket; /* Mémoriser le socket sur lequel répondre */
        g_scan_pending = 1;
        (void)osThreadFlagsSet(tid_Test, THREAD_FLAG_CMD);
        return 0;
    }

    /* 4. Toute commande inconnue reçoit ERR */
    udp_send_response(socket, addr, "ERR commande inconnue\n");
    return 0;
}

// Send UDP data to destination client.
void send_udp_data (void) {
  /* Non utilisé : le serveur répond directement aux requêtes des clients */
}

// Allocate and initialize the socket.
void Init_Ethernet (void) {

  netInitialize ();

  /* Ouvrir le port 5000 (cahier des charges et interface Qt) */
  udp_sock = netUDP_GetSocket (udp_cb_func);
  if (udp_sock > 0) {
    netUDP_Open (udp_sock, UDP_PORT_CDC);
  }
}

//! [code_UDP_Socket]
