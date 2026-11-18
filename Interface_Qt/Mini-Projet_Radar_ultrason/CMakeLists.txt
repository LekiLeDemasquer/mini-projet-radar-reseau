cmake_minimum_required(VERSION 3.19) # Version minimale de CMake requise
project(Mini-Projet_Radar_ultrason LANGUAGES CXX) # Nom du projet et langage C++

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Network) # Cherche les bibliotheques Qt6 Core, Widgets et Network

qt_standard_project_setup() # Configuration standard d'un projet Qt

qt_add_executable(Mini-Projet_Radar_ultrason # Cree l'executable avec la liste des fichiers du projet
    WIN32 MACOSX_BUNDLE # Options pour creer une application fenetree sous Windows/Mac
    main.cpp # Programme principal
    mainwindow.cpp # Code de la fenetre principale
    mainwindow.h # En-tete de la fenetre principale
    mainwindow.ui # Interface graphique Qt Designer
    udpthread.cpp # Code du thread UDP
    udpthread.h # En-tete du thread UDP
) # Fin de la liste des fichiers

target_link_libraries(Mini-Projet_Radar_ultrason # Relie les bibliotheques au projet
    PRIVATE # Visibilite privee
        Qt::Core # Bibliotheque de base Qt
        Qt::Widgets # Bibliotheque graphique Qt
        Qt::Network # Bibliotheque reseau integree a Qt (QUdpSocket, QHostAddress)
) # Fin des bibliotheques
