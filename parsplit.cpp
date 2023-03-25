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
 * @brief Prints array
 *
 * @param array Array to be printed
 * @param size Size of the array
 */
void print_array(int array[], int size)
{
    for (int i = 0; i < size; i++)
    {
        cout << array[i] << " ";
    }
}

/*
void print_LEG(int L[], int E[], int G[], int il, int ie, int ig, int rank)
{
    cout << "Rank " << rank << " L: ";
    print_array(L, il);
    cout << " E:";
    print_array(E, ie);
    cout << " G: ";
    print_array(G, ig);
    cout << endl;
}*/

/**
 * @brief Function that does the job -- splits array to LEG
 *
 * @param array Process working array
 * @param size Size of array
 * @param median Used median (usually not in array above)
 * @param L Array of values: lesser then median
 * @param E equal to median
 * @param G greater then median
 * @param il Pointer to size of array lesser then median
 * @param ie equal to median
 * @param ig greater then median
 */
void medianize(int array[], int size, int median,
               int L[], int E[], int G[], int *il, int *ie, int *ig)
{
    for (int i = 0; i < size; i++)
    {
        if (array[i] < median)
        {
            L[*il] = array[i];
            *il = *il + 1;
        }
        else if (array[i] == median)
        {
            E[*ie] = array[i];
            *ie = *ie + 1;
        }
        else
        {
            G[*ig] = array[i];
            *ig = *ig + 1;
        }
    }
}

/**
 * @brief Cunction gathers segments of L, E or G and returns it via result pointer (and result size).
 *
 * @param rank Process rank
 * @param num_proc Number of processors
 * @param segment Segment L, E or G
 * @param ix Size of segment L, E or G (il, ie or ig)
 * @param result Array for result
 * @param result_size Size of result
 */
void gather_segment(int rank, int num_proc, int segment[], int ix, int *result, int *result_size)
{
    int *sizes, offsets[num_proc];
    if (rank == 0)
    {
        sizes = (int *)malloc(num_proc * sizeof(int));
    }

    MPI_Gather(&ix, 1, MPI_INT, sizes, 1, MPI_INT, 0, MPI_COMM_WORLD); // Send all ixs to others, convert i to sizes for root
    if (rank == 0)
    {
        offsets[0] = 0; // Offset serves for describing, where starts each part of the next gatherv result array
        *result_size = sizes[0];
        for (int i = 1; i < num_proc; i++)
        {
            offsets[i] = *result_size;
            *result_size = *result_size + sizes[i];
        } // Sizes 1 5 0 6 -> offsets 0 1 6 6
    }

    // Everybody send results and root gathers all results from himself and his minions 
    MPI_Gatherv(segment, ix, MPI_INT, result, sizes, offsets, MPI_INT, 0, MPI_COMM_WORLD);
}

/**
 * @brief Function prints result of program
 *
 * @param rank Rank of processor
 * @param res Result array (for L, E or G)
 * @param res_size Size of result array
 * @param type Type of result (L, E or G)
 */
void print_res(int rank, int res[], int res_size, string type)
{
    if (rank == 0)
    {
        cout << "Pole " << type << ": ";
        print_array(res, res_size);
        cout << endl;
    }
}

/**
 * @brief Function gather results from subprocesses, concatenates it and print it to stdout
 *
 * @param rank Rank of calling process
 * @param num_proc Count of the processes
 * @param L Array of elements less then median
 * @param E Array of elements equal to median
 * @param G Array of elements grater to median
 * @param il Size of array of elements less then median
 * @param ie Equal to median
 * @param ig Greater then median
 */
void concat_gather(int rank, int num_proc, int size, int L[], int E[], int G[], int il, int ie, int ig)
{
    int *res = NULL;
    if (rank == 0)
    {
        res = (int *)malloc(size * sizeof(int));
    }
    int ril = 0, rie = 0, rig = 0;
    gather_segment(rank, num_proc, L, il, res, &ril); //
    print_res(rank, res, ril, "L");
    gather_segment(rank, num_proc, E, ie, res, &rie);
    print_res(rank, res, rie, "E");
    gather_segment(rank, num_proc, G, ig, res, &rig);
    print_res(rank, res, rig, "G");
}

/**
 * @brief Work of root process
 *
 * @param num_proc Number of processes
 */
void root_process(int num_proc)
{
    int array[MAXSIZE];
    unsigned char byte;
    int size_array = 0;
    int median;
    int chunk;

    FILE *file = fopen("numbers", "r");
    if (file == NULL)
    {
        printf("Nepovedlo se otevrit soubor\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    while (fscanf(file, "%c", &byte) != EOF && size_array < MAXSIZE)
    { // Loading input array from file
        array[size_array] = unsigned(byte);
        size_array++;
    }
    fclose(file);

    median = array[size_array % 2 == 0 ? (size_array / 2) - 1 : size_array / 2]; // Pseudomedian
    int small_size = size_array / num_proc;
    int buffer[size_array];

    cout << "Pseudomedian je: " << median << endl;
    cout << "Vstupni pole je: ";
    print_array(array, size_array);
    cout << endl;

    MPI_Bcast(&median, 1, MPI_INT, 0, MPI_COMM_WORLD); // Broadcasting median and size of array to others
    MPI_Bcast(&size_array, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Scatter(array, small_size, MPI_INT, buffer, small_size, MPI_INT, 0, MPI_COMM_WORLD); 
    // Send sub-arrays to others and take buffer with small_size

    int L[small_size], E[small_size], G[small_size];
    int il = 0, ie = 0, ig = 0;
    medianize(buffer, small_size, median, L, E, G, &il, &ie, &ig); // Do your part of job
    concat_gather(0, num_proc, size_array, L, E, G, il, ie, ig); // Gather all results
}

/**
 * @brief Work of non root processes
 *
 * @param num_proc Number of the processes
 * @param rank Rank of calling process
 */
void non_root_process(int num_proc, int rank)
{
    int median = 0;
    int size_array = 0;
    int buffer[MAXSIZE];
    int il = 0, ie = 0, ig = 0;

    MPI_Bcast(&median, 1, MPI_INT, 0, MPI_COMM_WORLD);     // Receiving median
    MPI_Bcast(&size_array, 1, MPI_INT, 0, MPI_COMM_WORLD); // Receiving size of array
    int small_size = size_array / num_proc;
    MPI_Scatter(NULL, 0, MPI_INT, buffer, small_size, MPI_INT, 0, MPI_COMM_WORLD); // Receiving process work -- buffer
    // small_size says expected size of received buffer

    int L[size_array], E[size_array], G[size_array];
    medianize(buffer, small_size, median, L, E, G, &il, &ie, &ig);  // Doing job
    concat_gather(rank, num_proc, size_array, L, E, G, il, ie, ig); // Sending results to root
}

/**
 * @brief Main function
 *
 * @param argc Number of program arguments
 * @param argv Strings of program arguments
 * @return int Exit code
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