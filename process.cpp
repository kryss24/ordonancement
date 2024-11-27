#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>
#include <string>
#include <sstream>
#include <gtk/gtk.h>

class Process
{
public:
    int pid;
    std::string name;
    int arrivalTime;
    int burstTime;
    int priority;
    int remainingTime;
    int waitingTime;
    int turnaroundTime;
    int responseTime;

    Process(int p, std::string n, int at, int bt, int pr = 0)
        : pid(p), name(n), arrivalTime(at), burstTime(bt),
          priority(pr), remainingTime(bt), waitingTime(0),
          turnaroundTime(0), responseTime(-1) {}
};

class Scheduler
{
private:
    std::vector<Process> processes;
    GtkWidget *entryProcesses;
    GtkWidget *entryArrivals;
    GtkWidget *entryDurations;
    GtkWidget *entryPriorities;
    GtkWidget *fifoRadio;
    GtkWidget *priorityRadio;
    GtkWidget *roundRobinRadio;
    GtkWidget *sjfpreemptiveRadio;
    GtkWidget *drawingArea;
    GtkWidget *treeView;
    GtkWidget *entryQuantum;

    GtkListStore *listStore;

    int quantum = 0;

public:
    void addProcess(Process p)
    {
        processes.push_back(p);
    }

    void calculateWaitingAndTurnaround(Process &process, int startTime, int endTime)
    {
        process.waitingTime = startTime - process.arrivalTime;
        process.turnaroundTime = endTime - process.arrivalTime;
    }

    int getLastProcessId() const
    {
        if (processes.empty())
        {
            return 0;
        }
        return processes.back().pid;
    }

    void clearProcesses()
    {
        processes.clear();
        processes.shrink_to_fit();
    }

    void FCFS()
    {
        std::sort(processes.begin(), processes.end(),
                  [](const Process &a, const Process &b)
                  {
                      return a.arrivalTime < b.arrivalTime;
                  });

        int currentTime = 0;
        for (auto &process : processes)
        {
            if (currentTime < process.arrivalTime)
            {
                currentTime = process.arrivalTime;
            }

            int startTime = currentTime;
            currentTime += process.burstTime;
            calculateWaitingAndTurnaround(process, startTime, currentTime);
        }
    }

    void RoundRobin(int quantum)
    {
        std::queue<Process *> readyQueue;
        int currentTime = 0;
        size_t i = 0;

        std::sort(processes.begin(), processes.end(),
                  [](const Process &a, const Process &b)
                  {
                      return a.arrivalTime < b.arrivalTime;
                  });

        std::vector<bool> firstResponse(processes.size(), true);

        while (i < processes.size() || !readyQueue.empty())
        {
            while (i < processes.size() && processes[i].arrivalTime <= currentTime)
            {
                readyQueue.push(&processes[i]);
                i++;
            }

            if (!readyQueue.empty())
            {
                Process *currentProcess = readyQueue.front();
                readyQueue.pop();

                if (firstResponse[currentProcess->pid - 1])
                {
                    currentProcess->responseTime = currentTime - currentProcess->arrivalTime;
                    firstResponse[currentProcess->pid - 1] = false;
                }

                int executionTime = std::min(quantum, currentProcess->remainingTime);
                currentTime += executionTime;
                currentProcess->remainingTime -= executionTime;

                while (i < processes.size() && processes[i].arrivalTime <= currentTime)
                {
                    readyQueue.push(&processes[i]);
                    i++;
                }

                if (currentProcess->remainingTime > 0)
                {
                    readyQueue.push(currentProcess);
                }
                else
                {
                    calculateWaitingAndTurnaround(*currentProcess,
                                                  currentTime - currentProcess->burstTime,
                                                  currentTime);
                }
            }
            else
            {
                currentTime++;
            }
        }
    }

    void PriorityScheduling()
    {
        std::queue<Process *> readyQueue;

        int currentTime = 0;
        size_t i = 0;

        while (i < processes.size() || !readyQueue.empty())
        {
            while (i < processes.size() && processes[i].arrivalTime <= currentTime)
            {
                readyQueue.push(&processes[i]);
                i++;
            }

            if (!readyQueue.empty())
            {
                Process *currentProcess = readyQueue.front();
                readyQueue.pop();

                if (currentProcess->responseTime == -1)
                {
                    currentProcess->responseTime = currentTime - currentProcess->arrivalTime;
                }

                currentProcess->remainingTime = 0;
                currentTime += currentProcess->burstTime;

                while (i < processes.size() && processes[i].arrivalTime <= currentTime)
                {
                    readyQueue.push(&processes[i]);
                    i++;
                }

                trier(readyQueue, 1);

                calculateWaitingAndTurnaround(*currentProcess,
                                              currentTime - currentProcess->burstTime,
                                              currentTime);
            }
            else
            {
                currentTime++;
            }
        }
    }

    void trier(std::queue<Process *> &q, int type)
    {
        std::vector<Process *> processes;

        // Extraire les éléments de la queue dans un vecteur
        while (!q.empty())
        {
            processes.push_back(q.front());
            q.pop();
        }

        // Trier les éléments dans le vecteur
        if (type == 0)
        {
            std::sort(processes.begin(), processes.end(),
                      [](const Process *a, const Process *b)
                      {
                          if (a->burstTime == b->burstTime)
                          {
                              return a->arrivalTime < b->arrivalTime;
                          }
                          return a->burstTime < b->burstTime;
                      });
        }
        else
        {
            std::sort(processes.begin(), processes.end(),
                      [](const Process *a, const Process *b)
                      {
                          if (a->burstTime == b->burstTime)
                          {
                              return a->priority < b->priority;
                          }
                          return a->priority < b->priority;
                      });
        }

        // Réinsérer les éléments triés dans la queue
        for (const auto &process : processes)
        {
            q.push(process);
        }
    }

    void SJFPreemptive()
    {
        std::queue<Process *> readyQueue;

        int currentTime = 0;
        size_t i = 0;

        while (i < processes.size() || !readyQueue.empty())
        {
            while (i < processes.size() && processes[i].arrivalTime <= currentTime)
            {
                readyQueue.push(&processes[i]);
                i++;
            }

            if (!readyQueue.empty())
            {
                Process *currentProcess = readyQueue.front();
                readyQueue.pop();

                if (currentProcess->responseTime == -1)
                {
                    currentProcess->responseTime = currentTime - currentProcess->arrivalTime;
                }

                currentProcess->remainingTime = 0;
                currentTime += currentProcess->burstTime;

                while (i < processes.size() && processes[i].arrivalTime <= currentTime)
                {
                    readyQueue.push(&processes[i]);
                    i++;
                }

                trier(readyQueue, 0);

                calculateWaitingAndTurnaround(*currentProcess,
                                              currentTime - currentProcess->burstTime,
                                              currentTime);
            }
            else
            {
                currentTime++;
            }
        }
    }

    void displayResults()
    {
        std::cout << "PID\tName\t\tArrival\t\tBurst\t\tPriority\t\tWaiting\t\tTurnaround\tResponse\n";
        for (const auto &process : processes)
        {
            std::cout << process.pid << "\t" << process.name << "\t\t" << process.arrivalTime << "\t\t"
                      << process.burstTime << "\t\t" << process.priority << "\t\t"
                      << process.waitingTime << "\t\t" << process.turnaroundTime << "\t\t"
                      << process.responseTime << "\n";
        }
    }

    void getInputValues()
    {
        const char *processesText = gtk_entry_get_text(GTK_ENTRY(entryProcesses));
        const char *arrivalsText = gtk_entry_get_text(GTK_ENTRY(entryArrivals));
        const char *durationsText = gtk_entry_get_text(GTK_ENTRY(entryDurations));
        const char *prioritiesText = gtk_entry_get_text(GTK_ENTRY(entryPriorities));
        const char *quantumText = gtk_entry_get_text(GTK_ENTRY(entryQuantum));

        int numProcesses = atoi(processesText);
        std::string arrivalTimes = arrivalsText;
        std::string burstTimes = durationsText;
        std::string priorities = prioritiesText;
        quantum = atoi(quantumText);

        std::stringstream arrivalStream(arrivalTimes);
        std::stringstream burstStream(burstTimes);
        std::stringstream priorityStream(priorities);
        std::string arrival, burst, priority;

        int count = getLastProcessId() + 1;

        while (std::getline(arrivalStream, arrival, ',') &&
               std::getline(burstStream, burst, ',') &&
               std::getline(priorityStream, priority, ','))
        {
            addProcess(Process(count++, "Processus " + std::to_string(count),
                               std::stoi(arrival), std::stoi(burst), std::stoi(priority)));
        }
    }

    void showAlertWithValues()
    {
        getInputValues();

        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(fifoRadio)))
        {
            FCFS();
        }
        else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(roundRobinRadio)))
        {
            RoundRobin(quantum);
        }
        else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sjfpreemptiveRadio)))
        {
            SJFPreemptive();
        }
        else
        {
            PriorityScheduling();
        }

        displayResults();
        updateTreeView();
        drawGrid();
    }

    void drawGrid()
    {
        gtk_widget_queue_draw(drawingArea);
    }

    static gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer data)
    {
        Scheduler *scheduler = static_cast<Scheduler *>(data); // Convertir les données

        // Dimensions de la grille
        const int cellWidth = 30; // Largeur de chaque cellule
        const int rowHeight = 20; // Hauteur de chaque rangée
        const int xOffset = 70;   // Décalage horizontal (pour les temps)
        const int yOffset = 50;   // Décalage vertical (pour commencer à dessiner)
        const int quantum = 7;    // Quantum de 4 unités de temps

        // Dessiner les en-têtes des temps
        cairo_set_source_rgb(cr, 0, 0, 0); // Texte en noir
        for (int t = 0; t <= 50; ++t)
        { // Ajustez 50 en fonction de la durée maximale
            std::string timeLabel = std::to_string(t);
            cairo_move_to(cr, xOffset + t * cellWidth + cellWidth / 4, yOffset - 10);
            cairo_show_text(cr, timeLabel.c_str());
        }

        // Dessiner les grilles et les processus
        int row = 0; // Rangée initiale
        for (const auto &process : scheduler->processes)
        {
            int startTime = process.arrivalTime + process.waitingTime; // Correction ici
            int endTime = startTime + process.burstTime;

            // Dessiner la barre de progression en segments de 4 cases
            for (int t = process.arrivalTime; t < endTime; t += quantum)
            {
                int segmentEnd = std::min(t + quantum, endTime);
                for (int i = t; i < segmentEnd; ++i)
                {
                    // Couleur de la barre de progression
                    if (i < startTime)
                    {
                        cairo_set_source_rgb(cr, 0.0, 1.0, 1.0); // Bleu
                    }
                    else
                    {
                        cairo_set_source_rgb(cr, 0.0, 0.0, 1.0); // Bleu
                    }
                    cairo_rectangle(cr, xOffset + i * cellWidth, yOffset + row * rowHeight, cellWidth, rowHeight);
                    cairo_fill(cr);
                }
            }

            // Dessiner les lignes de la grille
            cairo_set_source_rgb(cr, 0, 0, 0); // Couleur noire pour les lignes
            cairo_set_line_width(cr, 1);
            for (int t = 0; t <= 50; ++t)
            { // Ajustez 50 pour le temps maximal
                cairo_move_to(cr, xOffset + t * cellWidth, yOffset);
                cairo_line_to(cr, xOffset + t * cellWidth, yOffset + (row + 1) * rowHeight);
            }
            cairo_move_to(cr, xOffset, yOffset + (row + 1) * rowHeight);
            cairo_line_to(cr, xOffset + 50 * cellWidth, yOffset + (row + 1) * rowHeight); // Ligne horizontale
            cairo_stroke(cr);                                                             // Dessiner les lignes

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
        return FALSE;                // Indiquer que le dessin est terminé
        return FALSE;
    }

    void createGUI()
    {
        GtkWidget *window;                                                // Fenêtre principale
        GtkWidget *grid;                                                  // Grille pour organiser les widgets
        GtkWidget *typeFrame, *paramsFrame, *buttonsFrame, *resultsFrame; // Cadres pour les sections
        GtkWidget *typeBox, *paramsBox, *buttonsBox, *resultsBox;         // Boîtes pour les sections
        GtkWidget *btnSchedule, *btnReset;                                // Boutons pour ordonner et réinitialiser

        gtk_init(NULL, NULL); // Initialiser GTK

        window = gtk_window_new(GTK_WINDOW_TOPLEVEL);                          // Créer la fenêtre
        gtk_window_set_title(GTK_WINDOW(window), "Ordonnanceur de Processus"); // Titre de la fenêtre
        gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);             // Taille par défaut
        g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);  // Fermer l'application

        grid = gtk_grid_new();                          // Créer une nouvelle grille
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

        sjfpreemptiveRadio = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(fifoRadio), "SJFPreemptive");
        gtk_box_pack_start(GTK_BOX(typeBox), sjfpreemptiveRadio, FALSE, FALSE, 0);

        gtk_grid_attach(GTK_GRID(grid), typeFrame, 0, 0, 1, 1); // Ajouter le cadre à la grille

        // Cadre pour les paramètres
        paramsFrame = gtk_frame_new("Paramètres");
        paramsBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
        gtk_container_add(GTK_CONTAINER(paramsFrame), paramsBox);

        // Champs d'entrée pour les processus
        entryProcesses = gtk_entry_new();                                                         // Champ pour le nombre de processus
        gtk_entry_set_placeholder_text(GTK_ENTRY(entryProcesses), "Nombre de processus (ex: 4)"); // Texte d'indication
        gtk_box_pack_start(GTK_BOX(paramsBox), entryProcesses, FALSE, FALSE, 0);                  // Ajouter à la boîte

        entryArrivals = gtk_entry_new();                                                             // Champ pour les temps d'arrivée
        gtk_entry_set_placeholder_text(GTK_ENTRY(entryArrivals), "Dates de création (ex: 0,1,3,5)"); // Texte d'indication
        gtk_box_pack_start(GTK_BOX(paramsBox), entryArrivals, FALSE, FALSE, 0);                      // Ajouter à la boîte

        entryDurations = gtk_entry_new();                                                               // Champ pour les durées des processus
        gtk_entry_set_placeholder_text(GTK_ENTRY(entryDurations), "Durée des processus (ex: 9,2,5,6)"); // Texte d'indication
        gtk_box_pack_start(GTK_BOX(paramsBox), entryDurations, FALSE, FALSE, 0);                        // Ajouter à la boîte

        entryPriorities = gtk_entry_new();                                                    // Champ pour les priorités
        gtk_entry_set_placeholder_text(GTK_ENTRY(entryPriorities), "Priorité (ex: 1,2,3,4)"); // Texte d'indication
        gtk_box_pack_start(GTK_BOX(paramsBox), entryPriorities, FALSE, FALSE, 0);             // Ajouter à la boîte

        entryQuantum = gtk_entry_new();                                                    // Champ pour le quantum
        gtk_entry_set_placeholder_text(GTK_ENTRY(entryQuantum), "Entre le quantum temps"); // Texte d'indication
        gtk_box_pack_start(GTK_BOX(paramsBox), entryQuantum, FALSE, FALSE, 0);

        gtk_grid_attach(GTK_GRID(grid), paramsFrame, 1, 0, 1, 1); // Ajouter le cadre des paramètres à la grille

        // Cadre pour les boutons
        buttonsFrame = gtk_frame_new(NULL);
        buttonsBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
        gtk_container_add(GTK_CONTAINER(buttonsFrame), buttonsBox);

        btnSchedule = gtk_button_new_with_label("Ordonner");                           // Bouton pour ordonner
        g_signal_connect(btnSchedule, "clicked", G_CALLBACK(onScheduleClicked), this); // Connecter le signal

        btnReset = gtk_button_new_with_label("Réinitialiser");               // Bouton pour réinitialiser
        gtk_box_pack_start(GTK_BOX(buttonsBox), btnSchedule, TRUE, TRUE, 0); // Ajouter le bouton d'ordonnancement
        gtk_box_pack_start(GTK_BOX(buttonsBox), btnReset, TRUE, TRUE, 0);    // Ajouter le bouton de réinitialisation

        gtk_grid_attach(GTK_GRID(grid), buttonsFrame, 0, 1, 2, 1); // Ajouter le cadre des boutons à la grille

        // Ajout de la zone de dessin
        drawingArea = gtk_drawing_area_new();                             // Créer la zone de dessin
        gtk_widget_set_hexpand(drawingArea, TRUE);                        // Permet l'expansion horizontale
        gtk_widget_set_vexpand(drawingArea, TRUE);                        // Permet l'expansion verticale
        gtk_grid_attach(GTK_GRID(grid), drawingArea, 0, 2, 2, 1);         // Ajouter la zone de dessin à la grille
        g_signal_connect(drawingArea, "draw", G_CALLBACK(on_draw), this); // Connecter le signal de dessin

        // Ajout du tableau des résultats
        resultsFrame = gtk_frame_new("Résultats");                  // Cadre pour les résultats
        resultsBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);      // Boîte pour les résultats
        gtk_container_add(GTK_CONTAINER(resultsFrame), resultsBox); // Ajouter la boîte au cadre

        // Créer le modèle de liste pour le tableau
        listStore = gtk_list_store_new(9, G_TYPE_INT, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT); // Modèle de liste avec 9 colonnes

        // Créer le GtkTreeView
        treeView = gtk_tree_view_new_with_model(GTK_TREE_MODEL(listStore)); // Créer la vue d'arbre avec le modèle
        g_object_unref(listStore);                                          // Le modèle est maintenant détenu par le treeView

        // Créer les colonnes pour le tableau
        GtkCellRenderer *renderer = gtk_cell_renderer_text_new();                                               // Créer un renderer pour le texte
        GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes("PID", renderer, "text", 0, NULL); // Colonne pour PID
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeView), column);                                           // Ajouter la colonne à la vue

        column = gtk_tree_view_column_new_with_attributes("Name", renderer, "text", 1, NULL); // Colonne pour le nom
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeView), column);                         // Ajouter la colonne à la vue

        column = gtk_tree_view_column_new_with_attributes("Arrival", renderer, "text", 2, NULL); // Colonne pour le temps d'arrivée
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeView), column);                            // Ajouter la colonne à la vue

        column = gtk_tree_view_column_new_with_attributes("Burst", renderer, "text", 3, NULL); // Colonne pour le temps d'exécution
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeView), column);                          // Ajouter la colonne à la vue

        column = gtk_tree_view_column_new_with_attributes("Priority", renderer, "text", 4, NULL); // Colonne pour la priorité
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeView), column);                             // Ajouter la colonne à la vue

        column = gtk_tree_view_column_new_with_attributes("Waiting", renderer, "text", 5, NULL); // Colonne pour le temps d'attente
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeView), column);                            // Ajouter la colonne à la vue

        column = gtk_tree_view_column_new_with_attributes("Turnaround", renderer, "text", 6, NULL); // Colonne pour le temps de retour
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeView), column);                               // Ajouter la colonne à la vue

        // Ajouter le GtkTreeView au conteneur
        gtk_box_pack_start(GTK_BOX(resultsBox), treeView, TRUE, TRUE, 0); // Ajouter la vue d'arbre à la boîte des résultats

        // Ajouter le cadre des résultats à la grille
        gtk_grid_attach(GTK_GRID(grid), resultsFrame, 0, 3, 2, 1); // Ajouter le cadre des résultats à la grille

        gtk_widget_show_all(window); // Afficher tous les widgets de la fenêtre
        gtk_main();                  // Lancer la boucle principale de GTK
    }

    void updateTreeView()
    {
        gtk_list_store_clear(listStore);
        for (const auto &process : processes)
        {
            GtkTreeIter iter;
            gtk_list_store_append(listStore, &iter);
            gtk_list_store_set(listStore, &iter,
                               0, process.pid,
                               1, process.name.c_str(),
                               2, process.arrivalTime,
                               3, process.burstTime,
                               4, process.priority,
                               5, process.waitingTime,
                               6, process.turnaroundTime,
                               7, process.responseTime,
                               -1);
        }
    }

    static void onScheduleClicked(GtkWidget *widget, gpointer data)
    {
        Scheduler *scheduler = static_cast<Scheduler *>(data);
        scheduler->showAlertWithValues();
    }
};
int main()
{
    Scheduler scheduler;
    scheduler.createGUI();
    return 0;
}