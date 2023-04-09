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

#define MAXSIZE 32
#define MINSIZE 4
#define CLUSTERS 4
#define MAXITERATIONS 1000

using namespace std;

/**
 * @brief Prints array of floats
 *
 * @param array
 * @param size
 */
void print_array(float array[], int size)
{
    for (int i = 0; i < size; i++)
    {
        cout << array[i] << " ";
    }
}

/**
 * @brief Prints array of floats
 * 
 * @param array 
 * @param size 
 */
void print_array_int(int array[], int size)
{
    for (int i = 0; i < size; i++)
    {
        cout << array[i] << " ";
    }
}

/**
 * @brief Get the class of kmeans for point
 * 
 * @param means Current means of clusters
 * @param point Selected point
 * @return unsigned int class of selected point
 */
unsigned int get_kmeans_class(float means[CLUSTERS], float point)
{
    float min_diff = abs(means[0] - point);
    unsigned int kmean_class = 1;
    for (int i = 1; i < CLUSTERS; i++)
    {
        float diff = abs(means[i] - point);
        if (diff < min_diff)
        {
            kmean_class = i + 1;
            min_diff = diff;
        }
    }
    return kmean_class;
}

/**
 * @brief Prints 4-means output as assignement wants.
 * 
 * @param means Clusters means
 * @param size_array How many elements we have
 * @param array Array of elements
 * @param flags Map of classes of array above
 */
void print_k_means(float means[], int size_array, float *array, int flags[])
{
    for (int i = 0; i < CLUSTERS; i++)
    {
        cout << "[" << means[i] << "]: "; 
        for (int j = 0; j < size_array; j++)
        {
            if(i + 1 == flags[j])
            {
                cout << array[j] << " ";
            }
        }
        cout << endl;
    }
}

/**
 * @brief Function gathers resulting classes of classes and prints it to the output
 * 
 * @param my_cluster Value of class/cluster of processor
 * @param size_array 
 * @param array 
 * @param means Array of clusters means
 * @param rank Processor rank
 */
void gather_results(int my_cluster, int size_array, float *array, float means[], int rank)
{
    int *flags = NULL;
    if (rank == 0) 
    {
        flags = (int *)malloc(sizeof(int) * size_array);
    }
    MPI_Gather(&my_cluster, 1, MPI_INT, flags, 1, MPI_INT, 0, MPI_COMM_WORLD);
    if (rank == 0)
    {
        print_k_means(means, size_array, array, flags);
    }
    free(flags);
}

/**
 * @brief Function turns to zero positions of old values and turns to new values on new indices
 * 
 * @param cluster_counts Array for counting classes of points
 * @param cluster_values Array for counting values of points
 * @param value Value of point
 * @param old_index Value of old class (real_index = old_index - 1)
 * @param new_index Value of new class (from 1 to 4, so as above real_index = new_index - 1)
 */
void new_cluster_values(int cluster_counts[], float cluster_values[], int value, int old_index, int new_index)
{
    cluster_counts[old_index - 1] = 0;
    cluster_counts[new_index - 1] = 1;
    cluster_values[old_index - 1] = 0.0;
    cluster_values[new_index - 1] = value;
}

/**
 * @brief Function counts new cluster means.
 * 
 * @param count_sums How many values was predicted to each cluster
 * @param val_sums Sum of values of points predicted to each cluster
 * @param means Array for result
 */
void count_means(int count_sums[], float val_sums[], float means[])
{
    for(int i = 0; i < CLUSTERS; i++)
    {
        if(count_sums[i] != 0)
        {
            means[i] = val_sums[i] / count_sums[i];
        }
        else means[i] = 0;
    }
}

/**
 * @brief Main job function. Does 4-means.
 * 
 * @param means Array for means. Root has to initialize this array before.
 * @param size_array Size of input array
 * @param num_proc Number of processors
 * @param rank Rank of process
 * @param array Input array
 */
void k_means(float means[], int size_array, int num_proc, int rank, float *array)
{
    float my_value;
    bool diff;

    // Distribute array of means to other classes
    MPI_Bcast(&size_array, 1, MPI_INT, 0, MPI_COMM_WORLD);
    // Each process obtains 1 value.
    MPI_Scatter(array, 1, MPI_FLOAT, &my_value, 1, MPI_FLOAT, 0, MPI_COMM_WORLD);

    int my_cluster = 1, new_cluster = 0;
    int cluster_counts[CLUSTERS] = {}; // Initialize values to zeroes
    float cluster_values[CLUSTERS] = {};
    int *cluster_sums = NULL;
    float *cluster_val_sums = NULL;

    if(rank == 0)
    {
        cluster_sums = (int*)malloc(sizeof(int) * CLUSTERS);
        cluster_val_sums = (float*)malloc(sizeof(float) * CLUSTERS);
    }
    
    bool any_diff = true;
    int iteration = 0;

    new_cluster_values(cluster_counts, cluster_values, my_value, 1, 1); // sets initial membership to 1

    do // Main cycle of program
    {
        MPI_Bcast(means, CLUSTERS, MPI_FLOAT, 0, MPI_COMM_WORLD);
        new_cluster = get_kmeans_class(means, my_value);
        if (my_cluster != new_cluster) // I count cluster membership and it is different then cluster before.
        {
            diff = true;
            new_cluster_values(cluster_counts, cluster_values, my_value, my_cluster, new_cluster); // Swaps values in arrays
            my_cluster = new_cluster;
        }
        else // The membership is same
        {
            diff = false;
        }
        MPI_Allreduce(&diff, &any_diff, 1, MPI_CXX_BOOL, MPI_LOR, MPI_COMM_WORLD); // If there was no difference, stop.
        
        // For process 0 counts cluster_sums (number of points to each class)
        MPI_Reduce(cluster_counts, cluster_sums, CLUSTERS, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD); 
        // For process 0 counts cluster_val_sums, which contains sums of values of points by prediction to class
        MPI_Reduce(cluster_values, cluster_val_sums, CLUSTERS, MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD);
        MPI_Barrier(MPI_COMM_WORLD);
        if(rank == 0)
        {
            count_means(cluster_sums, cluster_val_sums, means);
        }
        iteration++;
    } while (any_diff && iteration < MAXITERATIONS);
    gather_results(my_cluster, size_array, array, means, rank);
    if(rank == 0)
    {
        free(cluster_sums);
        free(cluster_val_sums);
    }
    
}

/**
 * @brief Function for workaround done by root process.
 * 
 * @param num_proc 
 */
void root_process(int num_proc)
{
    float *array = (float *)malloc(sizeof(float) * MAXSIZE);
    char byte;
    int size_array = 0;
    int median;
    int chunk;
    float k1, k2, k3, k4;

    FILE *file = fopen("numbers", "r"); // Reading from numbers file
    if (file == NULL)
    {
        cout << "Nepovedlo se otevrit soubor\n";
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    while (fscanf(file, "%c", &byte) != EOF && size_array < MAXSIZE)
    { // Loading input array from file
        array[size_array] = byte;
        size_array++;
    }
    fclose(file);

    if (size_array < num_proc || num_proc < MINSIZE)
    {
        cout << "Prilis malo dat\n";
        MPI_Abort(MPI_COMM_WORLD, 2);
    }
    // print_array(array, size_array);
    float means[CLUSTERS] = {array[0], array[1], array[2], array[3]};
    k_means(means, size_array, num_proc, 0, array);
    free(array);
}

int main(int argc, char **argv)
{
    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0) // Root process does workaround, others starts as soon as possible.
    {
        root_process(size);
    }
    else
    {
        float means[CLUSTERS] = {};
        k_means(means, 0, size, rank, NULL);
    }

    MPI_Finalize();
    return 0;
}
