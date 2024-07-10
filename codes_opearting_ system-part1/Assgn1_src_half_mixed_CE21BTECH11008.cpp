#include <iostream>
#include <thread>
#include <pthread.h>
#include <vector>
#include <cmath>
#include <time.h>
#include <fstream>

#define number_of_core as nc
#define nc 8

using namespace std;
pthread_mutex_t mutex;

typedef struct
{
    int thread_no;
    int N;
    vector<vector<int>> *matrix;
    vector<vector<long long>> *ans;
    vector<vector<int>> *rows;
    vector<int> vector_row;
    int *thread_id;
    vector<double> *time;

} arguments;

void *matrix_mult(void *args)
{

    arguments *threadArgs = (arguments *)args;
    int N = threadArgs->N;
    int thread_no = threadArgs->thread_no;
    vector<vector<int>> &matrix = *(threadArgs->matrix);
    vector<vector<long long>> &ans = *(threadArgs->ans);
    vector<int> rowvector = threadArgs->vector_row;
    vector<double> &time = *(threadArgs->time);
    // starting the timen
    auto start_time = std::chrono::high_resolution_clock::now();
    vector<int>::iterator it;
    for (it = rowvector.begin(); it != rowvector.end(); it++)
    {

        {
            for (int j = 0; j < N; j++)
            {
                long long sum = 0;

                for (int k = 0; k < N; k++)
                {
                    sum = sum + matrix[*it][k] * matrix[k][j];
                }

                ans[*it][j] = sum;
                sum = 0;
            }
        }
    }

    // Calculate the duration in microseconds
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_time = end_time - start_time;

    time[thread_no] = elapsed_time.count();

    // pthread_mutex_lock(&mutex);
    // cout << "Time taken by " << thread_no << " is executing on core " << sched_getcpu() << " is " <<time[thread_no]*1000<<" ms "<<endl ;
    // pthread_mutex_unlock(&mutex);

    return NULL;
}

vector<vector<int>> readmatrix(int &N, int &K, int &C, string &file1)
{

    // making the instance of the ifstream named input and taking input as file1
    ifstream input(file1);

    // if file is already open by other application then will through the error

    if (!input.is_open())
    {
        cerr << "file opening failed ..." << endl;
        exit(EXIT_FAILURE);
    }

    // Read the number of rows and columns from the first line of the file
    input >> N >> K >> C;

    // makig 2d vector named matrix of size N * N
    vector<vector<int>> matrix(N, vector<int>(N));

    // Loop through the file and read each element into the matrix
    for (int i = 0; i < N; ++i)
    {
        for (int j = 0; j < N; ++j)
        {
            input >> matrix[i][j];
        }
    }

    // Closing the file
    input.close();

    return matrix;
}

int main()
{

    // here N represents size of matrix row and K represnt number of thread
    int N;
    int K;
    int C;

    string filename = "inp.txt";

    // passing N and F so that can be assigned
    vector<vector<int>> matrix = readmatrix(N, K, C, filename);
    vector<vector<long long>> ans(N, vector<long long>(N));

    // our matrix is the square matrix ------

    // making the time vector to note down the time of size K as for each thread and initilizing with 0 for avoiding any ambiguity
    vector<double> time(K, 0);
    cout << N << " " << K << " " << C << endl;

    int dist = ceil((float)N / K);
    vector<vector<int>> rows(K);

    pthread_t thread[K];
    arguments arg1[K];

    // distributing each rows to their resepective thread

    for (int i = 0; i < N; i++)
    {
        rows[i % K].push_back(i);
    }

    // number of the threads get by each processor
    int each_get = K / C;
    
    cout << "thread creating" << endl;

    

    for (int i = 0; i < K / 2; i++)
    {
        arg1[i].matrix = &matrix;
        arg1[i].ans = &ans;
        arg1[i].N = N;
        arg1[i].thread_no = i;
        arg1[i].vector_row = rows[i];
        arg1[i].thread_id = new int(i);
        arg1[i].time = &time;

        // arg1[i].thread_id = &i;   if you do this then it is not guarantee that you will
        // get right value becasue this is not the dedicated address and something can be written
        // on thsi address by other function -----
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        // cpu set cant be greater than C/2 i.e. total number of core /2
        // each i is divided my maximum_thread they can get --this will imply that from i to each_get+i-1
        // will get same core.

        if (each_get != 0)
            CPU_SET((i / each_get) % C, &cpuset);
        // assigning to the core 0 if each_get =0
        else
            CPU_SET((2 * i / K) % C, &cpuset);
        // making the attributes
        pthread_attr_t attr;

        // setting the setaffinity part of attribute to the fixed cpu from the cpuset
        pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpuset);
        if (pthread_create(thread + i, &attr, &matrix_mult, (void *)&arg1[i]) != 0)
            return 2;
    }

    // rest of thread that are distrubuted by scheduler ----
    for (int i = K/2; i < K; i++)
    {
        arg1[i].matrix = &matrix;
        arg1[i].ans = &ans;
        arg1[i].N = N;
        arg1[i].thread_no = i;
        arg1[i].vector_row = rows[i];
        arg1[i].thread_id = new int(i);
        arg1[i].time = &time;

        if (pthread_create(thread + i, NULL, &matrix_mult, (void *)&arg1[i]) != 0)
            return 2;
    }

    for (int i = 0; i < K; i++)
    {

        if (pthread_join(thread[i], NULL) != 0)
            return 3;
    }
   
     double sum1 = 0;
    for (int l = 0; l < K / 2; l++)
    {
        sum1 = sum1 + time[l];
        // cout << time[l] << endl;
    }

    // taking average of the time --by dividing by K(number of threads)
    std::cout << "Time taken by bounded thread: " << (sum1 * 2 * 1000) / K << " milliseconds" << std::endl;

    double sum2 = 0;
    for (int l = K / 2; l < K; l++)
    {
        sum2 = sum2 + time[l];
        // cout << time[l] << endl;
    }

    // taking average of the time --by dividing by K(number of threads)
    std::cout << "Time taken by unbounded thread : " << (sum2 * 1000 * 2) / K << " milliseconds" << std::endl;

    FILE *fptr;
    fptr = fopen("outfile.txt", "w");

    for (int i = 0; i < N; i++)
    {
        for (int j = 0; j < N; j++)
        {

            fprintf(fptr, "%lld ", ans[i][j]);
        }
        fprintf(fptr, "\n");
    }

    return 0;
}