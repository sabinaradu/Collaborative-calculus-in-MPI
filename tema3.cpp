#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>

#include <fstream>
#include <vector>

using namespace std;

void trimite_mesaj_int(int *package, int size, int sursa, int dest, int tag) {
    string result = "M(" + to_string(sursa) + ',' + to_string(dest) + ")";
    MPI_Send(package, size, MPI_INT, dest, tag, MPI_COMM_WORLD);
    cout << result << "\n";
}

void afiseaza_topologie(int rank, int coordonatori[][100], int *procese_coordonate) {
    string topologie = to_string(rank) + " ->";

    int i, j;
    for (i = 0; i < 3; ++i) {
        // Afisez coordonatorul
        topologie += ' ' + to_string(i) + ':';

        // Afisez procese coordonate
        for (j = 0; j < procese_coordonate[i] - 1; ++j) {
            topologie += to_string(coordonatori[i][j]) + ',';
        }
        topologie += to_string(coordonatori[i][j]);
    }

    cout << topologie << "\n";
}

int min_func(int a, int b) {
    return a < b ? a : b;
}

int main(int argc, char *argv[]) {
    int no_procs, rank;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &no_procs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Status status;

    // Declar listele de coordonatori ale fiecarui proces
    int coordonatori[3][100]; //ce workeri are un coordonator
    int procese_coord[3]{1, 1, 1}; // cati workeri are un coordonator
    int coordonator = -1;

    // Doar coordonatorii se apuca de citit datele de intrare
    if (rank < 3) {
        // Citire din fisier
        string filename = "cluster" + to_string(rank) + ".txt";
        ifstream fin(filename);

        fin >> procese_coord[rank];

        for (int i = 0; i < procese_coord[rank]; ++i) {
            fin >> coordonatori[rank][i];
        }
        fin.close();

        // Trimiterea topologiei proprii catre ceilalti coordonatori
        if(rank == 0) {
            // Trimmiterea topologiei proprii doar lui 2
            trimite_mesaj_int(&procese_coord[0], 1, 0, 2, 0);
            trimite_mesaj_int(coordonatori[0], procese_coord[0], 0, 2, 0);

            // Primesc topologia celorlalti coordonatori
            MPI_Recv(&procese_coord[2], 1, MPI_INT, 2, 0, MPI_COMM_WORLD, &status);
            MPI_Recv(coordonatori[2], procese_coord[2], MPI_INT, 2, 0, MPI_COMM_WORLD, &status);
            MPI_Recv(&procese_coord[1], 1, MPI_INT, 2, 0, MPI_COMM_WORLD, &status);
            MPI_Recv(coordonatori[1], procese_coord[1], MPI_INT, 2, 0, MPI_COMM_WORLD, &status);
        }

        if(rank == 2) {
            // Trimit topologia proprie celorlalti coordonatori
            trimite_mesaj_int(&procese_coord[2], 1, 2, 0, 0);
            trimite_mesaj_int(&procese_coord[2], 1, 2, 1, 0);
            trimite_mesaj_int(coordonatori[2], procese_coord[2], 2, 0, 0);
            trimite_mesaj_int(coordonatori[2], procese_coord[2], 2, 1, 0);

            //Primesc topologia lui 0 si o trimit lui 1
            MPI_Recv(&procese_coord[0], 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
            MPI_Recv(coordonatori[0], procese_coord[0], MPI_INT, 0, 0, MPI_COMM_WORLD, &status);

            trimite_mesaj_int(&procese_coord[0], 1, 2, 1, 0);
            trimite_mesaj_int(coordonatori[0], procese_coord[0], 2, 1, 0);

            //Primesc topologia lui 1 si o trimit lui 0
            MPI_Recv(&procese_coord[1], 1, MPI_INT, 1, 0, MPI_COMM_WORLD, &status);
            MPI_Recv(coordonatori[1], procese_coord[1], MPI_INT, 1, 0, MPI_COMM_WORLD, &status);

            trimite_mesaj_int(&procese_coord[1], 1, 2, 0, 0);
            trimite_mesaj_int(coordonatori[1], procese_coord[1], 2, 0, 0);
        }

        if(rank == 1) {
            //Trimit topologia proprie lui 2
            trimite_mesaj_int(&procese_coord[1], 1, 1, 2, 0);
            trimite_mesaj_int(coordonatori[1], procese_coord[1], 1, 2, 0);

            // Primesc topologia fiecarui coorodnator
            MPI_Recv(&procese_coord[2], 1, MPI_INT, 2, 0, MPI_COMM_WORLD, &status);
            MPI_Recv(coordonatori[2], procese_coord[2], MPI_INT, 2, 0, MPI_COMM_WORLD, &status);
            MPI_Recv(&procese_coord[0], 1, MPI_INT, 2, 0, MPI_COMM_WORLD, &status);
            MPI_Recv(coordonatori[0], procese_coord[0], MPI_INT, 2, 0, MPI_COMM_WORLD, &status);
        }

        // Se trimit catre toate procesele copil
        for (int j = 0; j < procese_coord[rank]; ++j) {
            trimite_mesaj_int(procese_coord, 3, rank, coordonatori[rank][j], 0);
            trimite_mesaj_int(coordonatori[0], procese_coord[0], rank, coordonatori[rank][j], 0);
            trimite_mesaj_int(coordonatori[1], procese_coord[1], rank, coordonatori[rank][j], 1);
            trimite_mesaj_int(coordonatori[2], procese_coord[2], rank, coordonatori[rank][j], 2);
        }

    } else {  // Se executa pentru workeri
        // Primire nr procese
        MPI_Recv(procese_coord, 3, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
        coordonator = status.MPI_SOURCE;

        // Primire topologie
        MPI_Recv(coordonatori[0], procese_coord[0], MPI_INT, coordonator, 0, MPI_COMM_WORLD, &status);
        MPI_Recv(coordonatori[1], procese_coord[1], MPI_INT, coordonator, 1, MPI_COMM_WORLD, &status);
        MPI_Recv(coordonatori[2], procese_coord[2], MPI_INT, coordonator, 2, MPI_COMM_WORLD, &status);
    }
    afiseaza_topologie(rank, coordonatori, procese_coord);

    // Task 2
    int v[50000], k = atoi(argv[1]);
    if (rank == 0) {
        for (int i = 0; i < k; ++i) {
            v[i] = i;
        }

        // Trimit mesajul catre coordonatorul 2
        trimite_mesaj_int(v, k, rank, 2, 0);

    } else if (rank == 2) {
        // Primesc informatia de la coordonatorul 0 o trimit lui 1
        MPI_Recv(v, k, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
        trimite_mesaj_int(v, k, rank, 1, 0);

    } else if(rank == 1) {
        // Primesc informatia de la coordonatorul 1 o trimit lui 2
        MPI_Recv(v, k, MPI_INT, 2, 0, MPI_COMM_WORLD, &status);
    }

    int thread_start = 9999999, thread_end = -1;
    if (rank < 3) {
        int index, process_rank;
        for (int j = 0; j < procese_coord[rank]; ++j) {
            // Formula offset inginerita pe loc
            process_rank = coordonatori[rank][j];
            if (rank == 0)
                index = j;
            if (rank == 1)
                index = j + procese_coord[0];
            if (rank == 2)
                index = j + procese_coord[0] + procese_coord[1];

            int start = index * (double)k / (no_procs - 3);
            int end = min_func(k, (index + 1) * k / (no_procs - 3));
            // Trimitere vector si bucata corespunzatoare
            trimite_mesaj_int(v, k, rank, process_rank, 0);
            trimite_mesaj_int(&start, 1, rank, process_rank, 1);
            trimite_mesaj_int(&end, 1, rank, process_rank, 2);
        }
        for (int j = 0; j < procese_coord[rank]; ++j) {
            // Bucata de cod trebuia copiata si rescrisa pentru a nu bloca codul cu MPI_Recv
            // Formula offset inginerita pe loc
            process_rank = coordonatori[rank][j];
            if (rank == 0)
                index = j;
            if (rank == 1)
                index = j + procese_coord[0];
            if (rank == 2)
                index = j + procese_coord[0] + procese_coord[1];

            int start = index * (double)k / (no_procs - 3);
            int end = min_func(k, (index + 1) * k / (no_procs - 3));

            if (start < thread_start) thread_start = start;
            if (end > thread_end) thread_end = end;

            // Receptare bucata calculata
            MPI_Recv(v + start, end - start, MPI_INT, process_rank, 0, MPI_COMM_WORLD, &status);
        }

        if(rank == 1) {
            //Trimit vectorul procesat lui 2
            trimite_mesaj_int(&thread_start, 1, rank, 2, 1);
            trimite_mesaj_int(&thread_end, 1, rank, 2, 2);
            trimite_mesaj_int(v + thread_start, thread_end - thread_start, rank, 2, 0);

        } else if(rank == 2) {
            // 2 va trimite vectorul procesat de workerii lui catre 0
            trimite_mesaj_int(&thread_start, 1, rank, 0, 1);
            trimite_mesaj_int(&thread_end, 1, rank, 0, 2);
            trimite_mesaj_int(v + thread_start, thread_end - thread_start, rank, 0, 0);

            //Primeste vectorul procesat de 1 si il trimite catre 0
            MPI_Recv(&thread_start, 1, MPI_INT, 1, 1, MPI_COMM_WORLD, &status);
            MPI_Recv(&thread_end, 1, MPI_INT, 1, 2, MPI_COMM_WORLD, &status);
            MPI_Recv(v + thread_start, thread_end - thread_start, MPI_INT, 1, 0, MPI_COMM_WORLD, &status);

            trimite_mesaj_int(&thread_start, 1, rank, 0, 1);
            trimite_mesaj_int(&thread_end, 1, rank, 0, 2);
            trimite_mesaj_int(v + thread_start, thread_end - thread_start, rank, 0, 0);

        } else if(rank == 0) {
            //Primeste vectorii procesati de ceilalti coordonatori
            MPI_Recv(&thread_start, 1, MPI_INT, 2, 1, MPI_COMM_WORLD, &status);
            MPI_Recv(&thread_end, 1, MPI_INT, 2, 2, MPI_COMM_WORLD, &status);
            MPI_Recv(v + thread_start, thread_end - thread_start, MPI_INT, 2, 0, MPI_COMM_WORLD, &status);

            MPI_Recv(&thread_start, 1, MPI_INT, 2, 1, MPI_COMM_WORLD, &status);
            MPI_Recv(&thread_end, 1, MPI_INT, 2, 2, MPI_COMM_WORLD, &status);
            MPI_Recv(v + thread_start, thread_end - thread_start, MPI_INT, 2, 0, MPI_COMM_WORLD, &status);
        }

    } else {
        int start, end;
        MPI_Recv(v, k, MPI_INT, coordonator, 0, MPI_COMM_WORLD, &status);
        MPI_Recv(&start, 1, MPI_INT, coordonator, 1, MPI_COMM_WORLD, &status);
        MPI_Recv(&end, 1, MPI_INT, coordonator, 2, MPI_COMM_WORLD, &status);

        for (int i = start; i < end; ++i) {
            v[i] *= 2;
        }

        // Trimitere vector si bucata corespunzatoare inapoi
        trimite_mesaj_int(v + start, end - start, rank, coordonator, 0);
    }

    // Afisez rezultatul
   if (rank == 0) {
        string result = "Rezultat:";
        for (int i = 0; i < k; ++i) {
            result += ' ' + to_string(v[i]);
        }
        /*result += '\n';*/
        cout << result << "\n";
    }


    MPI_Finalize();

    return 0;
}