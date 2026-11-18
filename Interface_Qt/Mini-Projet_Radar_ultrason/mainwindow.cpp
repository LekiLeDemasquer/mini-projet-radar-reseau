#include "mainwindow.h" // Inclut le fichier d'en-tete de notre fenetre MainWindow
#include "ui_mainwindow.h" // Inclut le fichier genere automatiquement par Qt a partir de mainwindow.ui

MainWindow::MainWindow(QWidget *parent) // Constructeur de la fenetre MainWindow
    : QMainWindow(parent) // Appelle le constructeur de la classe mere QMainWindow
    , ui(new Ui::MainWindow) // Cree l'interface graphique a partir du fichier mainwindow.ui
{ // Debut du constructeur
    ui->setupUi(this); // Place tous les boutons, afficheurs LCD et zones de texte sur la fenetre
    pUdpThread = new UdpThread; // Cree l'objet thread UDP en memoire (comme pCompteur = new C_compteur)
    connect(pUdpThread, SIGNAL(NewReponse(QString)), this, SLOT(GetNewReponse(QString))); // Relie le signal du thread au slot GetNewReponse de la fenetre
} // Fin du constructeur

MainWindow::~MainWindow() // Destructeur appele quand on ferme la fenetre
{ // Debut du destructeur
    pUdpThread->wait(); // Attend que le thread UDP ait fini son travail avant de fermer
    delete pUdpThread; // Libere la memoire utilisee par le thread UDP
    delete ui; // Libere la memoire de l'interface graphique
} // Fin du destructeur

void MainWindow::on_hsAngle_valueChanged(int value) // Fonction appelee quand on deplace le curseur (slider) hsAngle
{ // Debut de la fonction
    ui->lcdAngle->display(value); // Affiche la valeur de l'angle choisi sur l'afficheur LCD lcdAngle
} // Fin de la fonction

void MainWindow::on_pbAngle_clicked() // Fonction appelee quand on clique sur le bouton "Envoyer ANGLE"
{ // Debut de la fonction
    if (pUdpThread->isRunning()) // Verifie si le thread est deja en train d'envoyer une commande
    { // Debut du bloc si occupe
        return; // Ne fait rien pour eviter de lancer le thread deux fois en meme temps
    } // Fin du bloc si occupe

    int angle = ui->hsAngle->value(); // Recupere la valeur actuelle du curseur d'angle (entre 0 et 180)
    pUdpThread->m_ip = ui->lineEditIp->text(); // Copie l'adresse IP ecrite dans l'interface vers le thread
    pUdpThread->m_port = ui->spinBoxPort->value(); // Copie le numero de port choisi dans l'interface vers le thread
    pUdpThread->m_commande = "ANGLE " + QString::number(angle) + "\n"; // Fabrique la commande "ANGLE <valeur>\n" demandee par le cahier des charges

    ui->textEditReponse->append("> Envoi : ANGLE " + QString::number(angle)); // Affiche la commande envoyee dans la zone de texte
    pUdpThread->start(); // Demarre le thread (ce qui execute automatiquement UdpThread::run())
} // Fin de la fonction

void MainWindow::on_pbDist_clicked() // Fonction appelee quand on clique sur le bouton "Mesurer Distance (DIST)"
{ // Debut de la fonction
    if (pUdpThread->isRunning()) // Verifie si le thread est deja en train de travailler
    { // Debut du bloc si occupe
        return; // Quitte la fonction si une commande est deja en cours
    } // Fin du bloc si occupe

    pUdpThread->m_ip = ui->lineEditIp->text(); // Copie l'adresse IP ecrite dans l'interface vers le thread
    pUdpThread->m_port = ui->spinBoxPort->value(); // Copie le numero de port choisi dans l'interface vers le thread
    pUdpThread->m_commande = "DIST\n"; // Fabrique la commande "DIST\n" demandee par le cahier des charges

    ui->textEditReponse->append("> Envoi : DIST"); // Affiche la commande envoyee dans la zone de texte
    pUdpThread->start(); // Demarre le thread pour envoyer la requete UDP
} // Fin de la fonction

void MainWindow::on_pbScan_clicked() // Fonction appelee quand on clique sur le bouton "Lancer SCAN"
{ // Debut de la fonction
    if (pUdpThread->isRunning()) // Verifie si le thread est deja en train de travailler
    { // Debut du bloc si occupe
        return; // Quitte la fonction si une commande est deja en cours
    } // Fin du bloc si occupe

    int debut = ui->spinBoxDebut->value(); // Recupere l'angle de debut dans la case spinBoxDebut
    int fin = ui->spinBoxFin->value(); // Recupere l'angle de fin dans la case spinBoxFin
    int pas = ui->spinBoxPas->value(); // Recupere le pas d'avancement dans la case spinBoxPas

    pUdpThread->m_ip = ui->lineEditIp->text(); // Copie l'adresse IP ecrite dans l'interface vers le thread
    pUdpThread->m_port = ui->spinBoxPort->value(); // Copie le numero de port choisi dans l'interface vers le thread
    pUdpThread->m_commande = "SCAN " + QString::number(debut) + " " + QString::number(fin) + " " + QString::number(pas) + "\n"; // Fabrique la commande "SCAN <debut> <fin> <pas>\n"

    ui->textEditReponse->append("> Envoi : SCAN " + QString::number(debut) + " " + QString::number(fin) + " " + QString::number(pas)); // Affiche la commande envoyee dans la zone de texte
    pUdpThread->start(); // Demarre le thread pour envoyer la requete UDP
} // Fin de la fonction

void MainWindow::GetNewReponse(QString reponse) // Slot appele automatiquement quand le thread emet le signal NewReponse
{ // Debut de la fonction
    ui->textEditReponse->append("< Recu :\n" + reponse.trimmed()); // Affiche la reponse du STM32 dans la zone de texte

    if (reponse.startsWith("DIST ")) // Si la reponse commence par "DIST " (exemple : "DIST 42.5")
    { // Debut du bloc DIST
        QString valeurTexte = reponse.mid(5).trimmed(); // Recupere le texte apres les 5 premiers caracteres ("DIST ")
        if (valeurTexte == "TIMEOUT") // Si le capteur ultrason n'a pas recu d'echo ("DIST TIMEOUT")
        { // Debut du bloc TIMEOUT
            ui->lcdDistance->display(-1); // Affiche -1 sur l'afficheur LCD pour indiquer qu'il n'y a pas d'echo
        } // Fin du bloc TIMEOUT
        else // Sinon, c'est une mesure en centimetres
        { // Debut du bloc mesure valide
            double distanceCm = valeurTexte.toDouble(); // Convertit le texte recu en nombre a virgule (double)
            ui->lcdDistance->display(distanceCm); // Affiche la distance en cm sur l'afficheur LCD lcdDistance
        } // Fin du bloc mesure valide
    } // Fin du bloc DIST
} // Fin de la fonction
