/**
 * @file parsplit.cpp
 * @author David Hudak (xhudak03@vutbr.cz)
 * @brief Implementation of first project to PRL subject.
 * @version 1
 * @date 2023-03-24
 * 
 */

#include <iostream>
#include <mpi.h>

#define MAXSIZE 64
#define MINSIZE 8

using namespace std;

void root_process(unsigned array[])
{
    unsigned char byte;
    int i = 0;
    FILE* file = fopen("numbers", "r");
    if (file == NULL) {
        printf("Nepovedlo se otevrit soubor\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    
    while (fscanf(file, "%c", &byte) != EOF && i < 64) {
        array[i] = unsigned(byte);
        cout << unsigned(byte) << endl;
        i++;
    }
    fclose(file);
}

int main(int argc, char **argv)
{
    int rank;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0) {
        int input_array[MAXSIZE];
        root_process(input_array);
        cout << "Ja jsem root!" << endl; 
    }
    else
    {
        cout << "Ahoj!" << endl; 
        
    }

    MPI_Finalize();
    return 0;
    
}