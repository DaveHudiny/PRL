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
 * @brief
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

void print_array_int(int array[], int size)
{
    for (int i = 0; i < size; i++)
    {
        cout << array[i] << " ";
    }
}

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

void new_cluster_values(int cluster_counts[], float cluster_values[], int value, int old_index, int new_index)
{
    cluster_counts[old_index - 1] = 0;
    cluster_counts[new_index - 1] = 1;
    cluster_values[old_index - 1] = 0.0;
    cluster_values[new_index - 1] = value;
}

void count_means(int count_sums[], float val_sums[], float means[])
{
    for(int i = 0; i < CLUSTERS; i++)
    {
        means[i] = val_sums[i] / count_sums[i];
    }
}

void k_means(float means[], int size_array, int num_proc, int rank, float *array)
{
    float my_value;
    bool diff;
    MPI_Bcast(means, CLUSTERS, MPI_FLOAT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&size_array, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Scatter(array, 1, MPI_FLOAT, &my_value, 1, MPI_FLOAT, 0, MPI_COMM_WORLD);
    int my_cluster = 0;
    int new_cluster = 0;
    int cluster_counts[CLUSTERS] = {};
    float cluster_values[CLUSTERS] = {};
    bool any_diff;
    int iteration = 0;

    do
    {
        diff = false;

        new_cluster = get_kmeans_class(means, my_value);
        if (my_cluster != new_cluster)
        {
            diff = true;
            my_cluster = new_cluster;
        }
        MPI_Allreduce(&diff, &any_diff, 1, MPI_CXX_BOOL, MPI_LAND, MPI_COMM_WORLD);
        if (any_diff)
        {
            diff = true;
            new_cluster_values(cluster_counts, cluster_values, my_value, my_cluster, new_cluster);
            int cluster_sums[CLUSTERS];
            float cluster_val_sums[CLUSTERS];
            MPI_Reduce(cluster_counts, cluster_sums, CLUSTERS, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
            MPI_Reduce(cluster_values, cluster_val_sums, CLUSTERS, MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD);
            if(rank == 0)
            {
                count_means(cluster_sums, cluster_val_sums, means);
            }
            MPI_Bcast(means, CLUSTERS, MPI_FLOAT, 0, MPI_COMM_WORLD);
        }
        iteration++;
    } while (diff && iteration < MAXITERATIONS);

    gather_results(my_cluster, size_array, array, means, rank);
    
}

void root_process(int num_proc)
{
    float *array = (float *)malloc(sizeof(float) * MAXSIZE);
    char byte;
    int size_array = 0;
    int median;
    int chunk;
    float k1, k2, k3, k4;

    FILE *file = fopen("numbers", "r");
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
    if (rank == 0)
    {
        root_process(size);
    }
    else
    {
        float means[CLUSTERS] = {0.0, 0.0, 0.0, 0.0};
        k_means(means, 0, size, rank, NULL);
    }

    MPI_Finalize();
    return 0;
}
