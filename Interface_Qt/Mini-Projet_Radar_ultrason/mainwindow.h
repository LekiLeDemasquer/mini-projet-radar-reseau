/**
 * @file mainwindow.h
 * @brief Déclaration de la fenêtre principale Qt (contrôle angle, distance, scan UDP).
 */

#ifndef MAINWINDOW_H // Verifie si MAINWINDOW_H n'est pas encore defini (evite les inclusions multiples)
#define MAINWINDOW_H // Definit MAINWINDOW_H pour la suite de la compilation

#include "udpthread.h" // Inclut notre classe UdpThread qui gere la communication UDP
#include <QMainWindow> // Inclut la classe de base QMainWindow de Qt pour creer la fenetre

QT_BEGIN_NAMESPACE // Debut de l'espace de noms de Qt
namespace Ui { // Espace de noms Ui genere automatiquement a partir de mainwindow.ui
class MainWindow; // Declaration de la classe d'interface graphique MainWindow
} // Fin de l'espace de noms Ui
QT_END_NAMESPACE // Fin de l'espace de noms de Qt

class MainWindow : public QMainWindow // Declaration de notre classe MainWindow qui herite de QMainWindow
{ // Debut de la classe MainWindow
    Q_OBJECT // Macro obligatoire de Qt pour faire fonctionner SIGNAL et SLOT

public: // Methodes publiques de la classe
    explicit MainWindow(QWidget *parent = nullptr); // Constructeur de la fenetre principale
    ~MainWindow() override; // Destructeur de la fenetre principale

private slots: // Fonctions (slots) appelees lors des clics ou des signaux (comme dans tes exercices)
    void on_hsAngle_valueChanged(int value); // Slot appele automatiquement quand on bouge le curseur d'angle
    void on_pbAngle_clicked(); // Slot appele automatiquement quand on clique sur le bouton "Envoyer ANGLE"
    void on_pbDist_clicked(); // Slot appele automatiquement quand on clique sur le bouton "Mesurer Distance"
    void on_pbScan_clicked(); // Slot appele automatiquement quand on clique sur le bouton "Lancer SCAN"
    void GetNewReponse(QString reponse); // Slot appele quand le thread UdpThread a recu la reponse du STM32

private: // Variables privees de la fenetre
    Ui::MainWindow *ui; // Pointeur vers les elements graphiques dessines dans mainwindow.ui
    UdpThread *pUdpThread; // Pointeur vers notre thread UDP (comme pCompteur dans ton exercice IHM_Compteur)
}; // Fin de la classe MainWindow
#endif // MAINWINDOW_H // Fin de la condition #ifndef
