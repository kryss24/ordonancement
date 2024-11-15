#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>
#include <string>
#include <sstream> // Pour std::stringstream
#include <gtk/gtk.h> //

// Classe représentant un processus
class Process {
public:
    int pid;
    std::string name;
    int arrivalTime;
    int burstTime;
    int priority;
    int remainingTime;
    int waitingTime;
    int turnaroundTime;

    Process(int p, std::string n, int at, int bt, int pr = 0) 
        : pid(p), name(n), arrivalTime(at), burstTime(bt), 
          priority(pr), remainingTime(bt), waitingTime(0), turnaroundTime(0) {}
};

// Classe de gestion de l'ordonnancement
class Scheduler {
private:
    std::vector<Process> processes;

    // Widgets GTK pour les champs d'entrée
    GtkWidget *entryProcesses;
    GtkWidget *entryArrivals;
    GtkWidget *entryDurations;
    GtkWidget *entryPriorities;

    // Boutons radio pour sélectionner le type d'algorithme
    GtkWidget *fifoRadio;
    GtkWidget *priorityRadio;
    GtkWidget *roundRobinRadio;

    // Zone de dessin pour la grille
    GtkWidget *drawingArea;

    // Variables pour stocker les valeurs récupérées
    int numProcesses;
    std::string arrivalTimes;
    std::string burstTimes;
    std::string priorities;
    std::string selectedAlgorithm;
    std::string algorithm;

public:
    void addProcess(Process p) {
        processes.push_back(p);
    }

    void calculateWaitingAndTurnaround(Process& process, int startTime, int endTime) {
        process.waitingTime = startTime - process.arrivalTime;
        process.turnaroundTime = endTime - process.arrivalTime;
    }

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
        std::sort(processes.begin(), processes.end(), 
            [](const Process& a, const Process& b) {
                return a.arrivalTime < b.arrivalTime;
            });

        int currentTime = 0;
        for (auto& process : processes) {
            if (currentTime < process.arrivalTime)
                currentTime = process.arrivalTime;

            int startTime = currentTime;
            currentTime += process.burstTime;
            calculateWaitingAndTurnaround(process, startTime, currentTime);
        }
    }

    void RoundRobin(int quantum) {
        std::queue<Process*> readyQueue;
        int currentTime = 0;
        size_t i = 0;

        while (i < processes.size() || !readyQueue.empty()) {
            // Add arrived processes to the ready queue
            while (i < processes.size() && processes[i].arrivalTime <= currentTime) {
                readyQueue.push(&processes[i]);
                i++;
            }

            if (!readyQueue.empty()) {
                Process* currentProcess = readyQueue.front();
                readyQueue.pop();

                int executionTime = std::min(quantum, currentProcess->remainingTime);
                int startTime = currentTime;
                currentTime += executionTime;
                currentProcess->remainingTime -= executionTime;

                // Update waiting time for other processes in the queue
                for (auto& process : processes) {
                    if (process.pid != currentProcess->pid && process.arrivalTime <= currentTime) {
                        process.waitingTime += executionTime;
                    }
                }

                if (currentProcess->remainingTime > 0) {
                    readyQueue.push(currentProcess);
                } else {
                    calculateWaitingAndTurnaround(*currentProcess, startTime, currentTime);
                }
            } else {
                currentTime++;
            }
        }
    }

    // M�thode Priorit� (avec pr�emption)
    void PriorityScheduling() {
        // Trier les processus par temps d'arriv�e
        std::sort(processes.begin(), processes.end(),
                [](const Process& a, const Process& b) {
                    return a.arrivalTime < b.arrivalTime;
                });

        int currentTime = 0;
        std::vector<Process*> readyQueue;

        while (!processes.empty() || !readyQueue.empty()) {
            // Ajouter les processus qui sont arriv�s au temps actuel dans la file d'attente
            auto it = processes.begin();
            while (it != processes.end()) {
                if (it->arrivalTime <= currentTime) {
                    readyQueue.push_back(&(*it));
                    it = processes.erase(it);
                } else {
                    ++it;
                }
            }

            // Si la file d'attente est vide, avancer le temps
            if (readyQueue.empty()) {
                currentTime++;
                continue;
            }

            // Trouver le processus avec la plus haute priorit� dans la file d'attente
            auto highestPriority = std::min_element(readyQueue.begin(), readyQueue.end(),
                                                    [](const Process* a, const Process* b) {
                                                        return a->priority < b->priority;
                                                    });

            Process* currentProcess = *highestPriority;
            readyQueue.erase(highestPriority);

            // Simuler l'ex�cution du processus (1 unit� de temps pour permettre la pr�emption)
            currentTime++;
            currentProcess->remainingTime--;

            // Si le processus est termin�
            if (currentProcess->remainingTime == 0) {
                calculateWaitingAndTurnaround(*currentProcess,
                                            currentTime - currentProcess->burstTime,
                                            currentTime);
            } else {
                // R�ins�rer dans la file d'attente pour continuer plus tard
                readyQueue.push_back(currentProcess);
            }
        }
    }


    void displayResults() {
        std::cout << "PID\tName\t\tArrival\t\tBurst\t\tPriority\t\tWaiting\t\tTurnaround\n";
        for (const auto& process : processes) {
            std::cout << process.pid << "\t" << process.name << "\t\t" << process.arrivalTime << "\t\t" 
                      << process.burstTime << "\t\t" << process.priority << "\t\t" 
                      << process.waitingTime << "\t\t" << process.turnaroundTime << "\n";
        }
    }

    void getInputValues() {
        const char *processesText = gtk_entry_get_text(GTK_ENTRY(entryProcesses));
        const char *arrivalsText = gtk_entry_get_text(GTK_ENTRY(entryArrivals));
        const char *durationsText = gtk_entry_get_text(GTK_ENTRY(entryDurations));
        const char *prioritiesText = gtk_entry_get_text(GTK_ENTRY(entryPriorities));

        numProcesses = atoi(processesText);
        arrivalTimes = arrivalsText;
        burstTimes = durationsText;
        priorities = prioritiesText;

        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(fifoRadio))) {
            selectedAlgorithm = "FIFO";
        } else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(priorityRadio))) {
            selectedAlgorithm = "Priorité avec préemption";
        } else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(roundRobinRadio))) {
            selectedAlgorithm = "Tourniquet";
        }
    }

    void showAlertWithValues() {
        getInputValues();  // Récupérer les valeurs des champs d'entrée

        std::stringstream arrivalStream(arrivalTimes);
        std::stringstream burstStream(burstTimes);
        std::stringstream priorityStream(priorities);
        std::string arrival, burst, priority;

        int count = getLastProcessId() + 1;

        while (std::getline(arrivalStream, arrival, ',') &&
               std::getline(burstStream, burst, ',') &&
               std::getline(priorityStream, priority, ',')) {
            addProcess(Process(count++, "Processus " + std::to_string(count), 
                               std::stoi(arrival), std::stoi(burst), std::stoi(priority)));
        }
        std::cout << selectedAlgorithm << "\n";
        // Process::algorithm = selectedAlgorithm;
        
        if (selectedAlgorithm == "FIFO") {
            FCFS();
        } else if (selectedAlgorithm == "Tourniquet") {
            RoundRobin(4);
        } else {
            PriorityScheduling();
        }

        displayResults();

        drawGrid(); // Appel pour dessiner la grille après l'ordonnancement

    }

    void drawGrid() {
        gtk_widget_queue_draw(drawingArea); // Demande de redessiner la zone de dessin
    }

    // static gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer data) {
    //     Scheduler *scheduler = static_cast<Scheduler*>(data);

        // // Définir les dimensions et les marges pour la barre de progression
        // int barHeight = 20;
        // int barSpacing = 10;
        // int barStartX = 50; // Position de départ de la barre
        // int textOffsetY = 15; // Décalage pour le texte

        // // Trouver le temps total maximum pour ajuster les barres de progression proportionnellement
        // int maxTime = 0;
        // for (const auto& process : scheduler->processes) {
        //     maxTime = std::max(maxTime, process.turnaroundTime);
        //     std::cout << process.turnaroundTime << "\n";
        // }
        

        // // Parcourir chaque processus pour afficher le nom et la barre de progression
        // int yOffset = barSpacing; // Décalage vertical pour chaque barre
        // for (const auto& process : scheduler->processes) {
        //     // Afficher le nom du processus
        //     cairo_move_to(cr, 10, yOffset + textOffsetY);
        //     cairo_show_text(cr, process.name.c_str());

        //     // Calculer la largeur de la barre en fonction du temps d'exécution
        //     double barWidth = (process.turnaroundTime / (double)maxTime) * 300; // Ajuster 300 pour la longueur souhaitée

        //     // Définir la couleur de la barre (ici, en bleu)
        //     cairo_set_source_rgb(cr, 0.2, 0.6, 1.0);
            
        //     // Dessiner la barre de progression
        //     cairo_rectangle(cr, barStartX, yOffset, barWidth, barHeight);
        //     cairo_fill(cr);

        //     // Afficher le temps de turnaround sur la barre
        //     cairo_set_source_rgb(cr, 0, 0, 0); // Texte en noir
        //     cairo_move_to(cr, barStartX + barWidth + 5, yOffset + textOffsetY);
        //     cairo_show_text(cr, std::to_string(process.turnaroundTime).c_str());

        //     // Passer à la position suivante pour le prochain processus
        //     yOffset += barHeight + barSpacing;
        // }
    //     scheduler->clearProcesses();
    //     return FALSE;
    // }

    static gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer data) {
        Scheduler *scheduler = static_cast<Scheduler*>(data);

        // Dimensions de la grille
        int cellWidth = 30;  // Largeur de chaque cellule
        int rowHeight = 20;  // Hauteur de chaque rangée

        int xOffset = 100;   // Décalage horizontal (pour les temps)
        int yOffset = 50;    // Décalage vertical (pour commencer à dessiner)

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
            int startTime = process.arrivalTime + process.waitingTime - 1;
            int endTime = startTime + process.burstTime;

            // Couleur de la barre de progression
            double red = 0.2 + 0.1 * row;
            double green = 0.5;
            double blue = 1.0 - 0.1 * row;
            cairo_set_source_rgb(cr, red, green, blue);

            // Dessiner la barre de progression
            for (int t = startTime; t < endTime; ++t) {
                cairo_rectangle(cr, xOffset + t * cellWidth, yOffset + row * rowHeight, cellWidth, rowHeight);
                cairo_fill(cr);
            }

            // Dessiner les lignes de la grille
            cairo_set_source_rgb(cr, 0, 0, 0); // Couleur noire pour les lignes
            cairo_set_line_width(cr, 1);
            for (int t = 0; t <= 50; ++t) { // Ajustez 50 pour le temps maximal
                cairo_move_to(cr, xOffset + t * cellWidth, yOffset);
                cairo_line_to(cr, xOffset + t * cellWidth, yOffset + row * rowHeight);
            }
            cairo_move_to(cr, xOffset, yOffset + row * rowHeight);
            cairo_line_to(cr, xOffset + 50 * cellWidth, yOffset + row * rowHeight); // Ligne horizontale
            cairo_stroke(cr);

            // Ajouter le texte du nom du processus
            cairo_set_source_rgb(cr, 0, 0, 0); // Noir
            cairo_move_to(cr, 10, yOffset + row * rowHeight + rowHeight / 2);
            cairo_show_text(cr, process.name.c_str());

            row++; // Passer à la rangée suivante
        }

        // Dernière ligne horizontale pour fermer la grille
        cairo_set_source_rgb(cr, 0, 0, 0);
        cairo_move_to(cr, xOffset, yOffset + row * rowHeight);
        cairo_line_to(cr, xOffset + 50 * cellWidth, yOffset + row * rowHeight);
        cairo_stroke(cr);

        scheduler->clearProcesses();
        return FALSE;
    }



    void createGUI() {
        GtkWidget *window;
        GtkWidget *grid;
        GtkWidget *typeFrame, *paramsFrame, *buttonsFrame;
        GtkWidget *typeBox, *paramsBox, *buttonsBox;
        GtkWidget *btnSchedule, *btnReset;

        gtk_init(NULL, NULL);

        window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title(GTK_WINDOW(window), "Ordonnanceur de Processus");
        gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
        g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

        grid = gtk_grid_new();
        gtk_container_add(GTK_CONTAINER(window), grid);

        typeFrame = gtk_frame_new("Veuillez sélectionner votre type d'ordonnancement");
        typeBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
        gtk_container_add(GTK_CONTAINER(typeFrame), typeBox);

        fifoRadio = gtk_radio_button_new_with_label(NULL, "FIFO");
        gtk_box_pack_start(GTK_BOX(typeBox), fifoRadio, FALSE, FALSE, 0);

        priorityRadio = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(fifoRadio), "Priorité avec préemption");
        gtk_box_pack_start(GTK_BOX(typeBox), priorityRadio, FALSE, FALSE, 0);

        roundRobinRadio = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(fifoRadio), "Tourniquet");
        gtk_box_pack_start(GTK_BOX(typeBox), roundRobinRadio, FALSE, FALSE, 0);

        gtk_grid_attach(GTK_GRID(grid), typeFrame, 0, 0, 1, 1);

        paramsFrame = gtk_frame_new("Paramètres");
        paramsBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
        gtk_container_add(GTK_CONTAINER(paramsFrame), paramsBox);

        entryProcesses = gtk_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(entryProcesses), "Nombre de processus (ex: 4)");
        gtk_box_pack_start(GTK_BOX(paramsBox), entryProcesses, FALSE, FALSE, 0);

        entryArrivals = gtk_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(entryArrivals), "Dates de création (ex: 0,1,3,5)");
        gtk_box_pack_start(GTK_BOX(paramsBox), entryArrivals, FALSE, FALSE, 0);

        entryDurations = gtk_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(entryDurations), "Durée des processus (ex: 9,2,5,6)");
        gtk_box_pack_start(GTK_BOX(paramsBox), entryDurations, FALSE, FALSE, 0);

        entryPriorities = gtk_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(entryPriorities), "Priorité (ex: 1,2,3,4)");
        gtk_box_pack_start(GTK_BOX(paramsBox), entryPriorities, FALSE, FALSE, 0);

        gtk_grid_attach(GTK_GRID(grid), paramsFrame, 1, 0, 1, 1);

        buttonsFrame = gtk_frame_new(NULL);
        buttonsBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
        gtk_container_add(GTK_CONTAINER(buttonsFrame), buttonsBox);

        btnSchedule = gtk_button_new_with_label("Ordonner");
        g_signal_connect(btnSchedule, "clicked", G_CALLBACK(onScheduleClicked), this);
        
        btnReset = gtk_button_new_with_label("Réinitialiser");
        gtk_box_pack_start(GTK_BOX(buttonsBox), btnSchedule, TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(buttonsBox), btnReset, TRUE, TRUE, 0);

        gtk_grid_attach(GTK_GRID(grid), buttonsFrame, 0, 1, 2, 1);

        // Ajout de la zone de dessin
        drawingArea = gtk_drawing_area_new();
        gtk_widget_set_size_request(drawingArea, 800, 500);
        gtk_grid_attach(GTK_GRID(grid), drawingArea, 0, 2, 2, 1);
        g_signal_connect(drawingArea, "draw", G_CALLBACK(on_draw), this);

        gtk_widget_show_all(window);
        gtk_main();
    }

    static void onScheduleClicked(GtkWidget *widget, gpointer data) {
        Scheduler *scheduler = static_cast<Scheduler*>(data);
        scheduler->showAlertWithValues();
    }
};

int main() {
    Scheduler scheduler;

    // Lancement de l'interface graphique
    scheduler.createGUI();

    return 0;
}