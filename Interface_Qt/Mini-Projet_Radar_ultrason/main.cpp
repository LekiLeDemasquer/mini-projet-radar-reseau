/**
 * @file main.cpp
 * @brief Point d'entrée de l'interface graphique Qt du Radar à ultrasons.
 */

#include "mainwindow.h" // Inclut la declaration de notre fenetre principale MainWindow

#include <QApplication> // Inclut la classe QApplication qui gere l'application Qt

int main(int argc, char *argv[]) // Fonction principale (point d'entree du programme)
{ // Debut du programme principal
    QApplication a(argc, argv); // Cree l'objet application Qt
    MainWindow w; // Cree notre fenetre principale MainWindow
    w.show(); // Affiche la fenetre a l'ecran
    return QApplication::exec(); // Lance la boucle d'evenements de Qt (attend les clics de l'utilisateur)
} // Fin du programme principal
