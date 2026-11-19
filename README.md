# Mini-Projet Radar à Ultrasons Piloté par Réseau

Projet **BUT3 ESE** — Système embarqué de télémétrie radar orientable basé sur une carte **STM32F746G-DISCO** (Keil µVision + RTOS RTX5) et une interface de supervision PC développée en **Qt6 C++** communiquant en **UDP** (port **5000**).

---

## Architecture du Dépôt

```text
Mini_Projet_Radar/
├── Keil_µV/                   # Firmware embarqué STM32F746G-DISCO (Keil µVision, RTX5, AppWizard)
├── Interface_Qt/              # Application PC Qt6 C++ (QUdpSocket, thread dédié, supervision)
├── Banc_Test_STM32F103/       # Banc de test unitaire du capteur HC-SR04 sur STM32F103
├── Documentation/             # Cahier des charges, rapports d'analyse et guide de tests UDP
│   ├── Cahier_des_charges_Radar_ultrasons_reseau.pdf
│   ├── Guide_Tests_et_Fonctionnalites.md
│   ├── Rapport_Analyse_Mesure_Distance.md
│   └── images/
└── Livrable/                  # Dossier des livrables
```

---

## Matériel et Brochage (STM32F746G-DISCO)

| Périphérique | Signal | Broche STM32 | Connecteur Arduino | Configuration |
| :--- | :--- | :--- | :--- | :--- |
| **Servomoteur DM996** | **PWM (50 Hz)** | **PF6** | **A5** | `TIM10_CH1` (`AF3`), 1.0 ms (0°) à 2.0 ms (180°) |
| **Capteur HC-SR04** | **TRIG** | **PB14** | **D12** | Sortie Push-Pull, impulsion 10 µs |
| **Capteur HC-SR04** | **ECHO** | **PI0** | **D5** | Entrée Pull-Down (tolérante 5 V), mesure via `TIM6` (1 MHz) |
| **Réseau Ethernet** | **RJ45** | RMII | — | Pile Keil Network (`rl_net.h`), UDP port **5000** |
| **Alimentation** | **5 V / GND** | 5V / GND | CN6 | Alimentation 5 V externe recommandée pour le servo + masse commune |

---

## Protocole UDP (Port 5000)

La carte écoute les datagrammes ASCII sur le port UDP **5000** et exécute les commandes suivantes :

| Commande | Description | Réponse attendue |
| :--- | :--- | :--- |
| `ANGLE <0..180>` | Oriente le servomoteur à l'angle demandé, met à jour l'écran LCD puis confirme | `OK` |
| `DIST` | Déclenche une mesure ultrasonore HC-SR04 et affiche la distance sur le LCD | `DIST <cm>` ou `DIST TIMEOUT` |
| `SCAN <deb> <fin> <pas>` | Balaye de `<deb>` à `<fin>` par pas de `<pas>` (max 60 points) | `<angle> <cm>\n...END` |

Pour la liste complète des cas nominaux, des tests de robustesse et de la configuration réseau, consulter le [Guide de Tests et Fonctionnalités](Documentation/Guide_Tests_et_Fonctionnalites.md).

---

## Interface Graphique PC (Qt6)

- Communication UDP portable via `QtNetwork` (`QUdpSocket`) dans un thread dédié (`UdpThread`).
- Timeout automatique (2 s pour `ANGLE`/`DIST`, 15 s pour `SCAN`) avec 2 tentatives en cas de perte de paquet.
- Console temps réel affichant les commandes envoyées et les réponses reçues.

---

## Compilation et Utilisation Rapide

### 1. Firmware STM32 (Keil µVision)
1. Ouvrir le projet Keil dans `Keil_µV/`.
2. Compiler (`F7`) et flasher la carte **STM32F746G-DISCO** via ST-Link (`F8`).
3. L'écran LCD affiche l'adresse IP obtenue et l'angle courant (`<IPv4> | <Angle>`).

### 2. Interface PC Qt6
```bash
cd Interface_Qt/Mini-Projet_Radar_ultrason
cmake -B build
cmake --build build
```
Renseigner l'adresse IP de la carte STM32 dans l'interface (port UDP par défaut : **5000**).

### 3. Test rapide en ligne de commande
```bash
nc -u <IP_STM32> 5000
ANGLE 90
DIST
SCAN 0 180 30
```
