#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>
#include <string>
#include <sstream> // Pour std::stringstream
#include <gtk/gtk.h> // Bibliothèque GTK pour l'interface graphique

// Classe représentant un processus
class Process {
public:
    int pid; // Identifiant du processus
    std::string name; // Nom du processus
    int arrivalTime; // Temps d'arrivée du processus
    int burstTime; // Temps d'exécution du processus
    int priority; // Priorité du processus
    int remainingTime; // Temps restant pour l'exécution du processus
    int waitingTime; // Temps d'attente du processus
    int turnaroundTime; // Temps de retour du processus
    int responseTime; // Temps de réponse du processus

    // Constructeur de la classe Process
    Process(int p, std::string n, int at, int bt, int pr = 0)
        : pid(p), name(n), arrivalTime(at), burstTime(bt),
          priority(pr), remainingTime(bt), waitingTime(0),
          turnaroundTime(0), responseTime(-1) {} // Initialisation à -1 (non défini)
};

// Classe de gestion de l'ordonnancement
class Scheduler {
private:
    std::vector<Process> processes; // Vecteur pour stocker les processus

    // Widgets GTK pour les champs d'entrée
    GtkWidget *entryProcesses; // Champ pour le nombre de processus
    GtkWidget *entryArrivals; // Champ pour les temps d'arrivée
    GtkWidget *entryDurations; // Champ pour les durées des processus
    GtkWidget *entryPriorities; // Champ pour les priorités des processus

    // Boutons radio pour sélectionner le type d'algorithme
    GtkWidget *fifoRadio; // Radio pour FIFO
    GtkWidget *priorityRadio; // Radio pour Priorité
    GtkWidget *roundRobinRadio; // Radio pour Round Robin
    GtkWidget *sjfRadio, *sjfpreemptiveRadio; // Radios pour SJF

    // Zone de dessin pour la grille
    GtkWidget *drawingArea; // Zone de dessin pour visualiser l'ordonnancement

    // Variables pour stocker les valeurs récupérées
    int numProcesses; // Nombre de processus
    std::string arrivalTimes; // Chaîne pour les temps d'arrivée
    std::string burstTimes; // Chaîne pour les durées
    std::string priorities; // Chaîne pour les priorités
    std::string selectedAlgorithm; // Algorithme sélectionné
    std::string algorithm; // Algorithme

    // Widgets pour le tableau des résultats
    GtkWidget *treeView; // Vue d'arbre pour afficher les résultats
    GtkListStore *listStore; // Modèle de liste pour le tableau

public:
    // Méthode pour ajouter un processus au vecteur
    void addProcess(Process p) {
        processes.push_back(p);
    }

    // Méthode pour calculer le temps d'attente et le temps de retour d'un processus
    void calculateWaitingAndTurnaround(Process& process, int startTime, int endTime) {
        process.waitingTime = startTime - process.arrivalTime;
        process.turnaroundTime = endTime - process.arrivalTime;
    }

    // Méthode pour obtenir l'ID du dernier processus
    int getLastProcessId() const {
        if (processes.empty()) {
            return 0; // Retourne 0 si la liste est vide
        }
        return processes.back().pid; // Retourne l'ID du dernier processus
    }

    // Méthode pour vider le vecteur de processus
    void clearProcesses() {
        processes.clear(); // Vider le vecteur
        processes.shrink_to_fit(); // Optionnel : libérer la mémoire
    }

    // Méthode FCFS (First Come First Served)
    void FCFS() {
        // Trier les processus par temps d'arrivée
        std::sort(processes.begin(), processes.end(),
            [](const Process& a, const Process& b) {
                return a.arrivalTime < b.arrivalTime;
            });

        int currentTime = 0; // Temps actuel
        for (auto& process : processes) {
            // Avancer le temps si nécessaire
            if (currentTime < process.arrivalTime)
                currentTime = process.arrivalTime;

            int startTime = currentTime; // Temps de début d'exécution
            currentTime += process.burstTime; // Avancer le temps actuel
            calculateWaitingAndTurnaround(process, startTime, currentTime); // Calculer les temps
        }
    }

    // Méthode Round Robin
    void RoundRobin(int quantum) {
        std::queue<Process*> readyQueue; // File d'attente pour les processus prêts
        int currentTime = 0; // Temps actuel
        size_t i = 0; // Index pour parcourir les processus

        // Trier les processus par temps d'arrivée
        std::sort(processes.begin(), processes.end(),
                [](const Process& a, const Process& b) {
                    return a.arrivalTime < b.arrivalTime;
                });

        // Initialisation du tableau pour les temps de réponse
        std::vector<bool> firstResponse(processes.size(), true);

        while (i < processes.size() || !readyQueue.empty()) {
            // Ajouter les processus arrivés au temps actuel dans la file d'attente
            while (i < processes.size() && processes[i].arrivalTime <= currentTime) {
                readyQueue.push(&processes[i]);
                i++;
            }

            if (!readyQueue.empty()) {
                // Obtenir le processus en tête de file
                Process* currentProcess = readyQueue.front();
                readyQueue.pop();

                // Enregistrer le temps de réponse si c'est la première exécution
                if (firstResponse[currentProcess->pid - 1]) {
                    currentProcess->responseTime = currentTime - currentProcess->arrivalTime;
                    firstResponse[currentProcess->pid - 1] = false;
                }

                // Exécuter le processus pendant un quantum ou jusqu'à sa fin
                int executionTime = std::min(quantum, currentProcess->remainingTime);
                currentTime += executionTime; // Avancer le temps actuel
                currentProcess->remainingTime -= executionTime; // Réduire le temps restant

                // Ajouter les processus nouvellement arrivés pendant cette exécution
                while (i < processes.size() && processes[i].arrivalTime <= currentTime) {
                    readyQueue.push(&processes[i]);
                    i++;
                }

                // Si le processus n'est pas terminé, le remettre dans la file d'attente
                if (currentProcess->remainingTime > 0) {
                    readyQueue.push(currentProcess);
                } else {
                    // Si le processus est terminé, calculer les temps d'attente et de retour
                    calculateWaitingAndTurnaround(*currentProcess,
                                                currentTime - currentProcess->burstTime,
                                                currentTime);
                }
            } else {
                // Si la file d'attente est vide, avancer le temps
                currentTime++;
            }
        }
    }

    // Méthode pour l'ordonnancement par priorité
    void PriorityScheduling() {
        // Trier les processus par priorité en premier
        // Si deux processus ont la même priorité, comparer par temps d'arrivée
        std::sort(processes.begin(), processes.end(),
                    [](const Process& a, const Process& b) {
                        if (a.priority == b.priority) {
                            return a.arrivalTime < b.arrivalTime; // Comparer par temps d'arrivée
                        }
                        return a.priority < b.priority; // Comparer par priorité
                    });

        int currentTime = 0; // Temps actuel

        for (auto& process : processes) {
            // Avancer le temps si nécessaire
            if (currentTime < process.arrivalTime)
                currentTime = process.arrivalTime;

            int startTime = currentTime; // Temps de début d'exécution
            currentTime += process.burstTime; // Avancer le temps actuel
            calculateWaitingAndTurnaround(process, startTime, currentTime); // Calculer les temps
        }
    }

    // Méthode SJF (Shortest Job First)
    void SJF() {
        // Trier les processus par temps d'exécution croissant
        // Si deux processus ont le même temps d'exécution, trier par temps d'arrivée
        std::sort(processes.begin(), processes.end(),
                  [](const Process& a, const Process& b) {
                      if (a.burstTime == b.burstTime) {
                          return a.arrivalTime < b.arrivalTime; // Comparer par temps d'arrivée
                      }
                      return a.burstTime < b.burstTime; // Comparer par temps d'exécution
                  });

        int currentTime = 0; // Temps actuel

        for (auto& process : processes) {
            // Avancer le temps si nécessaire
            if (currentTime < process.arrivalTime) {
                currentTime = process.arrivalTime;
            }

            int startTime = currentTime; // Temps de début d'exécution
            currentTime += process.burstTime; // Avancer le temps actuel
            calculateWaitingAndTurnaround(process, startTime, currentTime); // Calculer les temps
        }
    }

    // Méthode SJF Preemptive
    void SJFPreemptive() {
        std::queue<Process*> readyQueue; // File d'attente pour les processus prêts
        int currentTime = 0; // Temps actuel
        size_t i = 0; // Index pour parcourir les processus

        // Trier les processus par temps d'arrivée
        std::sort(processes.begin(), processes.end(),
                [](const Process& a, const Process& b) {
                    return a.arrivalTime < b.arrivalTime;
                });

        // Initialisation du tableau pour les temps de réponse
        std::vector<bool> firstResponse(processes.size(), true);

        while (i < processes.size() || !readyQueue.empty()) {
            // Ajouter les processus arrivés au temps actuel dans la file d'attente
            while (i < processes.size() && processes[i].arrivalTime <= currentTime) {
                readyQueue.push(&processes[i]);
                i++;
            }

            if (!readyQueue.empty()) {
                // Trouver le processus avec le temps d'exécution restant le plus court
                auto shortestJobIt = std::min_element(readyQueue.front(), readyQueue.back(),
                    [](const Process& a, const Process& b) {
                        return a.remainingTime < b.remainingTime;
                    });

                Process* currentProcess = shortestJobIt; // Processus courant

                // Enregistrer le temps de réponse si c'est la première exécution
                if (firstResponse[currentProcess->pid - 1]) {
                    currentProcess->responseTime = currentTime - currentProcess->arrivalTime;
                    firstResponse[currentProcess->pid - 1] = false;
                }

                // Exécuter le processus pendant un quantum ou jusqu'à sa fin
                int executionTime = 1; // Exécuter une unité de temps
                currentTime += executionTime; // Avancer le temps actuel
                currentProcess->remainingTime -= executionTime; // Réduire le temps restant

                // Ajouter les processus nouvellement arrivés pendant cette exécution
                while (i < processes.size() && processes[i].arrivalTime <= currentTime) {
                    readyQueue.push(&processes[i]);
                    i++;
                }

                // Si le processus n'est pas terminé, le remettre dans la file d'attente
                if (currentProcess->remainingTime > 0) {
                    readyQueue.push(currentProcess);
                } else {
                    // Si le processus est terminé, calculer les temps d'attente et de retour
                    calculateWaitingAndTurnaround(*currentProcess,
                                                currentTime - currentProcess->burstTime,
                                                currentTime);
                }
            } else {
                // Si la file d'attente est vide, avancer le temps
                currentTime++;
            }
        }
    }

    // Méthode pour afficher les résultats
    void displayResults() {
        std::cout << "PID\tName\t\tArrival\t\tBurst\t\tPriority\t\tWaiting\t\tTurnaround\tResponse\n";
        for (const auto& process : processes) {
            std::cout << process.pid << "\t" << process.name << "\t\t" << process.arrivalTime << "\t\t"
                      << process.burstTime << "\t\t" << process.priority << "\t\t"
                      << process.waitingTime << "\t\t" << process.turnaroundTime << "\t\t"
                      << process.responseTime << "\n"; // Afficher les informations du processus
        }
    }

    // Méthode pour récupérer les valeurs d'entrée
    void getInputValues() {
        const char *processesText = gtk_entry_get_text(GTK_ENTRY(entryProcesses));
        const char *arrivalsText = gtk_entry_get_text(GTK_ENTRY(entryArrivals));
        const char *durationsText = gtk_entry_get_text(GTK_ENTRY(entryDurations));
        const char *prioritiesText = gtk_entry_get_text(GTK_ENTRY(entryPriorities));

        numProcesses = atoi(processesText); // Convertir le texte en entier
        arrivalTimes = arrivalsText; // Récupérer les temps d'arrivée
        burstTimes = durationsText; // Récupérer les durées
        priorities = prioritiesText; // Récupérer les priorités

        // Déterminer l'algorithme sélectionné
        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(fifoRadio))) {
            selectedAlgorithm = "FIFO";
        } else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(priorityRadio))) {
            selectedAlgorithm = "Priorité avec préemption";
        } else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(roundRobinRadio))) {
            selectedAlgorithm = "Tourniquet";
        } else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sjfRadio))) {
            selectedAlgorithm = "SJF";
        } else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sjfpreemptiveRadio))) {
            selectedAlgorithm = "SJFPreemptive";
        }
    }

    // Méthode pour afficher une alerte avec les valeurs récupérées
    void showAlertWithValues() {
        getInputValues();  // Récupérer les valeurs des champs d'entrée

        std::stringstream arrivalStream(arrivalTimes); // Flux pour les temps d'arrivée
        std::stringstream burstStream(burstTimes); // Flux pour les durées
        std::stringstream priorityStream(priorities); // Flux pour les priorités
        std::string arrival, burst, priority;

        int count = getLastProcessId() + 1; // Obtenir l'ID du prochain processus

        // Lire les valeurs des flux et ajouter les processus
        while (std::getline(arrivalStream, arrival, ',') &&
               std::getline(burstStream, burst, ',') &&
               std::getline(priorityStream, priority, ',')) {
            addProcess(Process(count++, "Processus " + std::to_string(count),
                               std::stoi(arrival), std::stoi(burst), std::stoi(priority)));
        }
        std::cout << selectedAlgorithm << "\n"; // Afficher l'algorithme sélectionné

        // Exécuter l'algorithme sélectionné
        if (selectedAlgorithm == "FIFO") {
            FCFS();
        } else if (selectedAlgorithm == "Tourniquet") {
            RoundRobin(4); // Quantum de 4 pour Round Robin
        } else if (selectedAlgorithm == "SJF") {
            SJF();
        } else if (selectedAlgorithm == "SJFPreemptive") {
            SJFPreemptive();
        } else {
            PriorityScheduling();
        }

        displayResults(); // Afficher les résultats
        updateTreeView(); // Mettre à jour la vue des résultats
        drawGrid(); // Appel pour dessiner la grille après l'ordonnancement
    }

    // Méthode pour dessiner la grille
    void drawGrid() {
        gtk_widget_queue_draw(drawingArea); // Demande de redessiner la zone de dessin
    }

    // Fonction de rappel pour dessiner dans la zone de dessin
    static gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer data) {
        Scheduler *scheduler = static_cast<Scheduler*>(data); // Convertir les données

        // Dimensions de la grille
        const int cellWidth = 30;  // Largeur de chaque cellule
        const int rowHeight = 20;  // Hauteur de chaque rangée
        const int xOffset = 70;    // Décalage horizontal (pour les temps)
        const int yOffset = 50;    // Décalage vertical (pour commencer à dessiner)
        const int quantum = 4;      // Quantum de 4 unités de temps

        // Dessiner les en-têtes des temps
        cairo_set_source_rgb(cr, 0, 0, 0); // Texte en noir
        for (int t = 0; t <= 50; ++t) {  // Ajustez 50 en fonction de la durée maximale
            std::string timeLabel = std::to_string(t);
            cairo_move_to(cr, xOffset + t * cellWidth + cellWidth / 4, yOffset - 10);
            cairo_show_text(cr, timeLabel.c_str());
        }

        // Dessiner les grilles et les processus
        int row = 0; // Rangée initiale
        for (const auto& process : scheduler->processes) {
            int startTime = process.arrivalTime + process.waitingTime; // Correction ici
            int endTime = startTime + process.burstTime;

            // Dessiner la barre de progression en segments de 4 cases
            for (int t = process.arrivalTime; t < endTime; t += quantum) {
                int segmentEnd = std::min(t + quantum, endTime);
                for (int i = t; i < segmentEnd; ++i) {
                    // Couleur de la barre de progression
                    if (i < startTime) {
                        cairo_set_source_rgb(cr, 0.0, 1.0, 1.0); // Bleu
                    } else {
                        cairo_set_source_rgb(cr, 0.0, 0.0, 1.0); // Bleu
                    }
                    cairo_rectangle(cr, xOffset + i * cellWidth, yOffset + row * rowHeight, cellWidth, rowHeight);
                    cairo_fill(cr);
                }
            }

            // Dessiner les lignes de la grille
            cairo_set_source_rgb(cr, 0, 0, 0); // Couleur noire pour les lignes
            cairo_set_line_width(cr, 1);
            for (int t = 0; t <= 50; ++t) { // Ajustez 50 pour le temps maximal
                cairo_move_to(cr, xOffset + t * cellWidth, yOffset);
                cairo_line_to(cr, xOffset + t * cellWidth, yOffset + (row + 1) * rowHeight);
            }
            cairo_move_to(cr, xOffset, yOffset + (row + 1) * rowHeight);
            cairo_line_to(cr, xOffset + 50 * cellWidth, yOffset + (row + 1) * rowHeight); // Ligne horizontale
            cairo_stroke(cr); // Dessiner les lignes

            // Ajouter le texte du nom du processus
            cairo_set_source_rgb(cr, 0, 0, 0); // Noir
            cairo_move_to(cr, 10, yOffset + row * rowHeight + rowHeight / 2);
            cairo_show_text(cr, process.name.c_str()); // Afficher le nom du processus

            row++; // Passer à la rangée suivante
        }

        // Dernière ligne horizontale pour fermer la grille
        cairo_set_source_rgb(cr, 0, 0, 0);
        cairo_move_to(cr, xOffset, yOffset + row * rowHeight);
        cairo_line_to(cr, xOffset + 50 * cellWidth, yOffset + row * rowHeight);
        cairo_stroke(cr); // Dessiner la dernière ligne

        scheduler->clearProcesses(); // Assurez-vous que cela est nécessaire
        return FALSE; // Indiquer que le dessin est terminé
    }

    // Méthode pour créer l'interface graphique
    void createGUI() {
        GtkWidget *window; // Fenêtre principale
        GtkWidget *grid; // Grille pour organiser les widgets
        GtkWidget *typeFrame, *paramsFrame, *buttonsFrame, *resultsFrame; // Cadres pour les sections
        GtkWidget *typeBox, *paramsBox, *buttonsBox, *resultsBox; // Boîtes pour les sections
        GtkWidget *btnSchedule, *btnReset; // Boutons pour ordonner et réinitialiser

        gtk_init(NULL, NULL); // Initialiser GTK

        window = gtk_window_new(GTK_WINDOW_TOPLEVEL); // Créer la fenêtre
        gtk_window_set_title(GTK_WINDOW(window), "Ordonnanceur de Processus"); // Titre de la fenêtre
        gtk_window_set_default_size(GTK_WINDOW(window), 800, 600); // Taille par défaut
        g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL); // Fermer l'application

        grid = gtk_grid_new(); // Créer une nouvelle grille
        gtk_container_add(GTK_CONTAINER(window), grid); // Ajouter la grille à la fenêtre

        // Cadre pour sélectionner le type d'ordonnancement
        typeFrame = gtk_frame_new("Veuillez sélectionner votre type d'ordonnancement");
        typeBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
        gtk_container_add(GTK_CONTAINER(typeFrame), typeBox);

        // Boutons radio pour les algorithmes
        fifoRadio = gtk_radio_button_new_with_label(NULL, "FIFO");
        gtk_box_pack_start(GTK_BOX(typeBox), fifoRadio, FALSE, FALSE, 0);

        priorityRadio = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(fifoRadio), "Priorité avec préemption");
        gtk_box_pack_start(GTK_BOX(typeBox), priorityRadio, FALSE, FALSE, 0);

        roundRobinRadio = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(fifoRadio), "Tourniquet");
        gtk_box_pack_start(GTK_BOX(typeBox), roundRobinRadio, FALSE, FALSE, 0);

        sjfRadio = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(fifoRadio), "SJF");
        gtk_box_pack_start(GTK_BOX(typeBox), sjfRadio, FALSE, FALSE, 0);

        sjfpreemptiveRadio = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(fifoRadio), "SJFPreemptive");
        gtk_box_pack_start(GTK_BOX(typeBox), sjfpreemptiveRadio, FALSE, FALSE, 0);

        gtk_grid_attach(GTK_GRID(grid), typeFrame, 0, 0, 1, 1); // Ajouter le cadre à la grille

        // Cadre pour les paramètres
        paramsFrame = gtk_frame_new("Paramètres");
        paramsBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
        gtk_container_add(GTK_CONTAINER(paramsFrame), paramsBox);

        // Champs d'entrée pour les processus
        entryProcesses = gtk_entry_new(); // Champ pour le nombre de processus
        gtk_entry_set_placeholder_text(GTK_ENTRY(entryProcesses), "Nombre de processus (ex: 4)"); // Texte d'indication
        gtk_box_pack_start(GTK_BOX(paramsBox), entryProcesses, FALSE, FALSE, 0); // Ajouter à la boîte

        entryArrivals = gtk_entry_new(); // Champ pour les temps d'arrivée
        gtk_entry_set_placeholder_text(GTK_ENTRY(entryArrivals), "Dates de création (ex: 0,1,3,5)"); // Texte d'indication
        gtk_box_pack_start(GTK_BOX(paramsBox), entryArrivals, FALSE, FALSE, 0); // Ajouter à la boîte

        entryDurations = gtk_entry_new(); // Champ pour les durées des processus
        gtk_entry_set_placeholder_text(GTK_ENTRY(entryDurations), "Durée des processus (ex: 9,2,5,6)"); // Texte d'indication
        gtk_box_pack_start(GTK_BOX(paramsBox), entryDurations, FALSE, FALSE, 0); // Ajouter à la boîte

        entryPriorities = gtk_entry_new(); // Champ pour les priorités
        gtk_entry_set_placeholder_text(GTK_ENTRY(entryPriorities), "Priorité (ex: 1,2,3,4)"); // Texte d'indication
        gtk_box_pack_start(GTK_BOX(paramsBox), entryPriorities, FALSE, FALSE, 0); // Ajouter à la boîte

        gtk_grid_attach(GTK_GRID(grid), paramsFrame, 1, 0, 1, 1); // Ajouter le cadre des paramètres à la grille

        // Cadre pour les boutons
        buttonsFrame = gtk_frame_new(NULL);
        buttonsBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
        gtk_container_add(GTK_CONTAINER(buttonsFrame), buttonsBox);

        btnSchedule = gtk_button_new_with_label("Ordonner"); // Bouton pour ordonner
        g_signal_connect(btnSchedule, "clicked", G_CALLBACK(onScheduleClicked), this); // Connecter le signal

        btnReset = gtk_button_new_with_label("Réinitialiser"); // Bouton pour réinitialiser
        gtk_box_pack_start(GTK_BOX(buttonsBox), btnSchedule, TRUE, TRUE, 0); // Ajouter le bouton d'ordonnancement
        gtk_box_pack_start(GTK_BOX(buttonsBox), btnReset, TRUE, TRUE, 0); // Ajouter le bouton de réinitialisation

        gtk_grid_attach(GTK_GRID(grid), buttonsFrame, 0, 1, 2, 1); // Ajouter le cadre des boutons à la grille

        // Ajout de la zone de dessin
        drawingArea = gtk_drawing_area_new(); // Créer la zone de dessin
        gtk_widget_set_hexpand(drawingArea, TRUE);  // Permet l'expansion horizontale
        gtk_widget_set_vexpand(drawingArea, TRUE);  // Permet l'expansion verticale
        gtk_grid_attach(GTK_GRID(grid), drawingArea, 0, 2, 2, 1); // Ajouter la zone de dessin à la grille
        g_signal_connect(drawingArea, "draw", G_CALLBACK(on_draw), this); // Connecter le signal de dessin

        // Ajout du tableau des résultats
        resultsFrame = gtk_frame_new("Résultats"); // Cadre pour les résultats
        resultsBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5); // Boîte pour les résultats
        gtk_container_add(GTK_CONTAINER(resultsFrame), resultsBox); // Ajouter la boîte au cadre

        // Créer le modèle de liste pour le tableau
        listStore = gtk_list_store_new(9, G_TYPE_INT, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT); // Modèle de liste avec 9 colonnes

        // Créer le GtkTreeView
        treeView = gtk_tree_view_new_with_model(GTK_TREE_MODEL(listStore)); // Créer la vue d'arbre avec le modèle
        g_object_unref(listStore); // Le modèle est maintenant détenu par le treeView

        // Créer les colonnes pour le tableau
        GtkCellRenderer *renderer = gtk_cell_renderer_text_new(); // Créer un renderer pour le texte
        GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes("PID", renderer, "text", 0, NULL); // Colonne pour PID
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeView), column); // Ajouter la colonne à la vue

        column = gtk_tree_view_column_new_with_attributes("Name", renderer, "text", 1, NULL); // Colonne pour le nom
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeView), column); // Ajouter la colonne à la vue

        column = gtk_tree_view_column_new_with_attributes("Arrival", renderer, "text", 2, NULL); // Colonne pour le temps d'arrivée
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeView), column); // Ajouter la colonne à la vue

        column = gtk_tree_view_column_new_with_attributes("Burst", renderer, "text", 3, NULL); // Colonne pour le temps d'exécution
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeView), column); // Ajouter la colonne à la vue

        column = gtk_tree_view_column_new_with_attributes("Priority", renderer, "text", 4, NULL); // Colonne pour la priorité
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeView), column); // Ajouter la colonne à la vue

        column = gtk_tree_view_column_new_with_attributes("Waiting", renderer, "text", 5, NULL); // Colonne pour le temps d'attente
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeView), column); // Ajouter la colonne à la vue

        column = gtk_tree_view_column_new_with_attributes("Turnaround", renderer, "text", 6, NULL); // Colonne pour le temps de retour
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeView), column); // Ajouter la colonne à la vue

        column = gtk_tree_view_column_new_with_attributes("Response", renderer, "text", 7, NULL); // Colonne pour le temps de réponse
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeView), column); // Ajouter la colonne à la vue

        // Ajouter le GtkTreeView au conteneur
        gtk_box_pack_start(GTK_BOX(resultsBox), treeView, TRUE, TRUE, 0); // Ajouter la vue d'arbre à la boîte des résultats

        // Ajouter le cadre des résultats à la grille
        gtk_grid_attach(GTK_GRID(grid), resultsFrame, 0, 3, 2, 1); // Ajouter le cadre des résultats à la grille

        gtk_widget_show_all(window); // Afficher tous les widgets de la fenêtre
        gtk_main(); // Lancer la boucle principale de GTK
    }

    // Méthode pour mettre à jour le GtkTreeView avec les résultats
    void updateTreeView() {
        gtk_list_store_clear(listStore); // Effacer les anciennes données

        // Ajouter les nouvelles données
        for (const auto& process : processes) {
            GtkTreeIter iter; // Itérateur pour le modèle de liste
            gtk_list_store_append(listStore, &iter); // Ajouter une nouvelle ligne
            gtk_list_store_set(listStore, &iter,
                               0, process.pid, // PID
                               1, process.name.c_str(), // Nom
                               2, process.arrivalTime, // Temps d'arrivée
                               3, process.burstTime, // Temps d'exécution
                               4, process.priority, // Priorité
                               5, process.waitingTime, // Temps d'attente
                               6, process.turnaroundTime, // Temps de retour
                               7, process.responseTime, // Temps de réponse
                               -1); // Fin des paramètres
        }
    }

    // Fonction de rappel pour le clic sur le bouton d'ordonnancement
    static void onScheduleClicked(GtkWidget *widget, gpointer data) {
        Scheduler *scheduler = static_cast<Scheduler*>(data); // Convertir les données
        scheduler->showAlertWithValues(); // Appeler la méthode pour afficher les valeurs
    }
};

// Fonction principale
int main() {
    Scheduler scheduler; // Créer une instance de Scheduler

    // Lancement de l'interface graphique
    scheduler.createGUI(); // Appeler la méthode pour créer l'interface
    return 0; // Fin du programme
}