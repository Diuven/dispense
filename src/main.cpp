#include <iostream>
#include <ctime>
#include <cstdlib>
#include <random>
#include <chrono>
#include <thread>
#include <tuple>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <utility>

#include "utils/mathlib.hpp"

void simpleMul(Matrix &A, Matrix &B, Matrix &C)
{
    for (int i = 0; i < A.rows; i++)
    {
        if (i % 100 == 0)
            std::cout << "i: " << i << std::endl;
        for (int j = 0; j < B.cols; j++)
        {
            int sum = 0;
            for (int k = 0; k < A.cols; k++)
            {
                sum += A.get(i, k) * B.get(k, j);
            }
            C.set(i, j, sum);
        }
    }
}

void simpleParallelMul(Matrix &A, Matrix &B, Matrix &C, int numThreads)
{
    std::thread threads[numThreads];
    for (int i = 0; i < numThreads; i++)
    {
        threads[i] = std::thread([&, i]()
                                 {
            for (int j = i; j < A.rows; j += numThreads)
            {
                for (int k = 0; k < B.cols; k++)
                {
                    int sum = 0;
                    for (int l = 0; l < A.cols; l++)
                    {
                        sum += A.get(j, l) * B.get(l, k);
                    }
                    C.set(j, k, sum);
                }
            } });
    }

    for (int i = 0; i < numThreads; i++)
    {
        threads[i].join();
    }
}

typedef std::tuple<int, Vector, Vector> TaskInputType;
typedef std::tuple<int, int> TaskOutputType;

std::queue<TaskInputType> taskBuffer;
std::mutex taskMtx;
std::condition_variable taskCv;

std::queue<TaskOutputType> resultBuffer;
std::mutex resultMtx;
std::condition_variable resultCv;

std::atomic<bool> stopFlag(false);

TaskOutputType consumeTask(TaskInputType &input)
{
    int sum = 0, task_id = std::get<0>(input);
    Vector &X = std::get<1>(input);
    Vector &Y = std::get<2>(input);
    for (int i = 0; i < X.size; i++)
    {
        sum += X.get(i) * Y.get(i);
    }
    return std::make_tuple(task_id, sum);
}

void consumer(int index)
{
    std::cout << "Consumer " << index << " started" << std::endl;
    while (!stopFlag)
    {
        TaskInputType task = std::make_tuple(0, Vector(0), Vector(0));
        {
            std::unique_lock<std::mutex> lck(taskMtx);

            taskCv.wait(lck, []
                        { return !taskBuffer.empty(); });
            task = taskBuffer.front();
            taskBuffer.pop();
        }
        if (std::get<0>(task) == -1)
        {
            break;
        }
        TaskOutputType result = consumeTask(task);
        {
            std::unique_lock<std::mutex> lck(resultMtx);
            resultBuffer.push(result);
        }
    }
}

void simpleDistributedMul(Matrix &A, Matrix &B, Matrix &C, int numThreads)
{

    // spawn consumer threads
    std::thread consumerThreads[numThreads];
    for (int i = 0; i < numThreads; i++)
    {
        consumerThreads[i] = std::thread(consumer, i);
    }

    Vector rows[A.rows]; // A.
    Vector cols[B.cols]; // B.

    for (int i = 0; i < A.rows; i++)
    {
        rows[i] = A.getRow(i);
    }
    for (int i = 0; i < B.cols; i++)
    {
        cols[i] = B.getCol(i);
    }

    for (int i = 0; i < A.rows; i++)
    {
        for (int j = 0; j < B.cols; j++)
        {
            int task_id = i * A.cols + j;
            TaskInputType task = std::make_tuple(task_id, rows[i], cols[j]);
            {
                std::unique_lock<std::mutex> lck(taskMtx);
                taskBuffer.push(task);
            }
            taskCv.notify_one();
        }
    }

    int taskCount = 0;
    while (taskCount < A.rows * B.cols)
    {
        if (taskCount % 50000 == 0)
            std::cout << "taskCount: " << taskCount << std::endl;

        TaskOutputType result = std::make_tuple(0, 0);
        {
            std::unique_lock<std::mutex> lck(resultMtx);
            resultCv.wait(lck, []
                          { return !resultBuffer.empty(); });
            result = resultBuffer.front();
            resultBuffer.pop();
        }
        int task_id = std::get<0>(result);
        int sum = std::get<1>(result);
        int i = task_id / A.cols;
        int j = task_id % A.cols;
        C.set(i, j, sum);
        taskCount++;
    }

    stopFlag = true;
    for (int i = 0; i < numThreads; i++)
    {
        TaskInputType task = std::make_tuple(-1, Vector(0), Vector(0));
        {
            std::unique_lock<std::mutex> lck(taskMtx);
            taskBuffer.push(task);
        }
        taskCv.notify_one();
    }
    for (int i = 0; i < numThreads; i++)
    {
        consumerThreads[i].join();
    }
}

int main()
{
    srand(time(0));

    Matrix A = randomMatrix(VECTOR_SIZE, VECTOR_SIZE);
    Matrix B = randomMatrix(VECTOR_SIZE, VECTOR_SIZE);

    std::cout << "Start\n";

    //

    Matrix R1(VECTOR_SIZE, VECTOR_SIZE);
    auto start = std::chrono::high_resolution_clock::now();
    // simpleMul(A, B, R1);
    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> diff = end - start;
    std::cout << "simpleMul Time: " << diff.count() << " s" << std::endl;

    // //

    Matrix R2(VECTOR_SIZE, VECTOR_SIZE);
    start = std::chrono::high_resolution_clock::now();
    simpleParallelMul(A, B, R2, 4);
    end = std::chrono::high_resolution_clock::now();

    diff = end - start;
    std::cout << "simpleParallelMul Time: " << diff.count() << " s" << std::endl;

    //

    Matrix R3(VECTOR_SIZE, VECTOR_SIZE);
    start = std::chrono::high_resolution_clock::now();
    simpleDistributedMul(A, B, R3, 4);
    end = std::chrono::high_resolution_clock::now();

    diff = end - start;
    std::cout << "simpleDistributedMul Time: " << diff.count() << " s" << std::endl;

    return 0;
}