/**
 * @file udpthread.h
 * @brief Déclaration du thread Qt dédié à l'envoi/réception des commandes UDP vers le STM32.
 */

#ifndef UDPTHREAD_H // Verifie si UDPTHREAD_H n'est pas encore defini (evite d'inclure le fichier deux fois)
#define UDPTHREAD_H // Definit UDPTHREAD_H pour la suite de la compilation

#include <QThread> // Inclut la classe QThread de Qt pour creer un thread
#include <QString> // Inclut la classe QString de Qt pour manipuler du texte

class UdpThread : public QThread // Cree la classe UdpThread qui herite de QThread (comme C_compteur dans ton exercice)
{ // Debut de la declaration de la classe
    Q_OBJECT // Macro obligatoire de Qt pour pouvoir utiliser les signaux (SIGNAL) et les slots (SLOT)

public: // Partie publique accessible depuis MainWindow
    explicit UdpThread(QObject *parent = nullptr); // Constructeur de notre thread
    virtual void run(); // Fonction principale du thread qui s'execute quand on appelle start()

    QString m_ip; // Variable pour stocker l'adresse IP du microcontroleur (ex: "192.168.1.156")
    int m_port; // Variable pour stocker le numero de port UDP (ex: 5000)
    QString m_commande; // Variable pour stocker la commande a envoyer (ex: "ANGLE 90\n")

signals: // Liste des signaux que le thread peut envoyer a la fenetre MainWindow
    void NewReponse(QString reponse); // Signal emis quand une reponse (ou une erreur) est recue
}; // Fin de la declaration de la classe

#endif // UDPTHREAD_H // Fin de la condition #ifndef
