/**
 * @file udpthread.cpp
 * @brief Implémentation du thread client UDP (validation IP/port, envoi avec 2 essais et timeout).
 */

#include "udpthread.h" // Inclut le fichier d'en-tete de notre classe UdpThread
#include <QUdpSocket> // Inclut la classe QUdpSocket de Qt pour la communication UDP
#include <QHostAddress> // Inclut la classe QHostAddress de Qt pour gerer l'adresse IP
#include <QByteArray> // Inclut la classe QByteArray de Qt pour stocker les octets envoyes et recus

UdpThread::UdpThread(QObject *parent) // Constructeur de la classe UdpThread
    : QThread{parent} // Appelle le constructeur de la classe mere QThread
{ // Debut du constructeur
    m_ip = "192.168.1.156"; // Initialise l'adresse IP par defaut a 192.168.1.156
    m_port = 5000; // Initialise le port par defaut a 5000 (cahier des charges)
    m_commande = ""; // Initialise la commande avec un texte vide
} // Fin du constructeur

void UdpThread::run() // Fonction executee en arriere-plan quand on lance pUdpThread->start()
{ // Debut de la fonction run()
    QHostAddress adresseServeur; // Objet Qt qui represente l'adresse IP du STM32 (ou du routeur)
    if (!adresseServeur.setAddress(m_ip.trimmed())) // Verifie si l'adresse IP saisie est valide
    { // Debut du bloc d'erreur adresse invalide
        emit NewReponse("ERREUR : Adresse IP invalide"); // Envoie un signal d'erreur a l'interface
        return; // Quitte la fonction run()
    } // Fin du bloc d'erreur adresse invalide

    if ((m_port <= 0) || (m_port > 65535)) // Verifie que le numero de port UDP est bien compris entre 1 et 65535
    { // Debut du bloc d'erreur port invalide
        emit NewReponse("ERREUR : Port UDP invalide"); // Envoie un signal d'erreur a l'interface
        return; // Quitte la fonction run()
    } // Fin du bloc d'erreur port invalide

    QUdpSocket socketUdp; // Cree la socket UDP Qt dans ce thread (fermee automatiquement a la fin de run())
    if (!socketUdp.bind(QHostAddress::AnyIPv4)) // Lie la socket a un port local libre en IPv4 pour pouvoir recevoir la reponse
    { // Debut du bloc d'erreur bind
        emit NewReponse("ERREUR : Impossible d'ouvrir la socket UDP (" + socketUdp.errorString() + ")"); // Signale l'erreur a l'interface
        return; // Quitte la fonction run()
    } // Fin du bloc d'erreur bind

    int timeout_ms; // Variable pour regler le temps d'attente maximum (timeout) en millisecondes
    if (m_commande.startsWith("SCAN")) // Verifie si la commande commence par le mot "SCAN"
    { // Debut du bloc si c'est un SCAN
        timeout_ms = 15000; // Met un delai d'attente de 15000 ms (15 secondes) car le balayage du servo est long
    } // Fin du bloc si c'est un SCAN
    else // Sinon (pour les commandes rapides ANGLE et DIST)
    { // Debut du bloc sinon
        timeout_ms = 2000; // Met un delai d'attente de 2000 ms (2 secondes)
    } // Fin du bloc sinon

    QByteArray donneesEnvoi = m_commande.toUtf8(); // Convertit la commande texte en tableau d'octets ASCII/UTF-8
    quint16 portServeur = static_cast<quint16>(m_port); // Convertit le numero de port au type non signe 16 bits attendu par Qt

    for (int essai = 1; essai <= 2; essai++) // Boucle qui fait jusqu'a 2 essais si le STM32 ne repond pas (demande par le cahier des charges)
    { // Debut de la boucle des essais
        qint64 octetsEnvoyes = socketUdp.writeDatagram(donneesEnvoi, adresseServeur, portServeur); // Envoie le datagramme UDP vers le STM32
        if (octetsEnvoyes == -1) // Verifie si l'envoi du datagramme a echoue
        { // Debut du bloc d'erreur d'envoi
            socketUdp.close(); // Ferme proprement la socket UDP
            emit NewReponse("ERREUR : Echec de l'envoi UDP (" + socketUdp.errorString() + ")"); // Signale l'erreur d'envoi a l'interface
            return; // Quitte la fonction run()
        } // Fin du bloc d'erreur d'envoi

        if (socketUdp.waitForReadyRead(timeout_ms)) // Attend l'arrivee d'une reponse UDP pendant au maximum timeout_ms millisecondes
        { // Debut du bloc si des donnees sont arrivees
            while (socketUdp.hasPendingDatagrams()) // Tant qu'il y a un datagramme UDP en attente de lecture
            { // Debut de la boucle de lecture
                QByteArray donneesRecues; // Tableau d'octets pour stocker la reponse du STM32
                donneesRecues.resize(static_cast<int>(socketUdp.pendingDatagramSize())); // Ajuste la taille du tableau a celle du datagramme recu
                QHostAddress adresseEmetteur; // Variable pour recuperer l'adresse IP de l'expediteur
                quint16 portEmetteur = 0; // Variable pour recuperer le port de l'expediteur

                qint64 octetsRecus = socketUdp.readDatagram(donneesRecues.data(), donneesRecues.size(), &adresseEmetteur, &portEmetteur); // Lit le datagramme recu
                if (octetsRecus > 0) // Si on a bien recu au moins 1 octet
                { // Debut du bloc de reception reussie
                    socketUdp.close(); // Ferme proprement la socket UDP
                    emit NewReponse(QString::fromUtf8(donneesRecues)); // Envoie la reponse recue a MainWindow grace au signal NewReponse
                    return; // Quitte la fonction run() car tout a fonctionne
                } // Fin du bloc de reception reussie
            } // Fin de la boucle de lecture
        } // Fin du bloc si des donnees sont arrivees
    } // Fin de la boucle des essais

    socketUdp.close(); // Ferme proprement la socket UDP (si les 2 essais ont echoue)
    emit NewReponse("ERREUR : Pas de reponse du STM32 (Timeout)"); // Signale a l'interface que le STM32 n'a pas repondu
} // Fin de la fonction run()
