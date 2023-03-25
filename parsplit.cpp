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

/**
 * @brief 
 * 
 * @param array 
 * @param size 
 */
void print_array(int array[], int size)
{
    for (int i = 0; i < size; i++)
    {
        cout << array[i] << " ";
    }
}

/**
 * @brief 
 * 
 * @param array 
 * @param size 
 * @param median 
 * @param rank 
 * @param num_proc 
 */
void medianize(int array[], int size, int median, int rank, int num_proc)
{
    int L[size], E[size], G[size];
    int il = 0, ie = 0, ig = 0;

    for (int i = 0; i < size; i++)
    {
        if (array[i] < median)
        {
            L[il] = array[i];
            il++;
        }
        else if (array[i] == median)
        {
            E[ie] = array[i];
            ie++;
        }
        else
        {
            G[ig] = array[i];
            ig++;
        }
    }
    /**
    print_array(L, il); 
    cout << " E:";
    print_array(E, ie);
    cout << " G: ";
    print_array(G, ig);
    cout << endl;
    **/
}

/**
 * @brief 
 * 
 * @param num_proc 
 */
void root_process(int num_proc)
{
    int array[MAXSIZE];
    unsigned char byte;
    int size_array = 0, i = 0;
    int median;
    int chunk;
    FILE *file = fopen("numbers", "r");
    if (file == NULL)
    {
        printf("Nepovedlo se otevrit soubor\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    while (fscanf(file, "%c", &byte) != EOF && i < 64)
    {
        array[i] = unsigned(byte);
        // cout << unsigned(byte) << " ";
        i++;
    }
    fclose(file);

    size_array = i;
    median = array[size_array % 2 == 0 ? (size_array / 2) - 1 : size_array / 2];
    int buffer[size_array];

    MPI_Bcast(&median, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&size_array, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Scatter(array, size_array / num_proc, MPI_INT, buffer, size_array / num_proc, MPI_INT, 0, MPI_COMM_WORLD);

    cout << "Proces " << 0 << ": ";
    print_array(buffer, size_array / num_proc);
    cout << " Median: " << median << endl;
    medianize(buffer, size_array / num_proc, median, 0, num_proc);
}

/**
 * @brief 
 * 
 * @param num_proc 
 * @param rank 
 */
void non_root_process(int num_proc, int rank)
{
    int median = 0;
    int array_size = 0;
    int buffer[MAXSIZE];
    MPI_Bcast(&median, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&array_size, 1, MPI_INT, 0, MPI_COMM_WORLD);

    MPI_Scatter(NULL, array_size / num_proc, MPI_INT, buffer, array_size / num_proc, MPI_INT, 0, MPI_COMM_WORLD);
    cout << "Proces " << rank << ": ";
    print_array(buffer, array_size / num_proc);
    cout << endl;
    medianize(buffer, array_size / num_proc, median, 0, num_proc);
}

/**
 * @brief 
 * 
 * @param argc 
 * @param argv 
 * @return int 
 */
int main(int argc, char **argv)
{
    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0)
    {
        root_process(size);
    }
    else
    {
        non_root_process(size, rank);
    }

    MPI_Finalize();
    return 0;
}