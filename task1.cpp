#include <iostream>
#include <mutex>
#include <thread>
#include <chrono>
#include <vector>
#define COUNT 20000

using namespace std;
std::mutex mut;

double* x; // вектор, на который умножается матрица
double* M; // матрица
double* res; //результат умножения

void define(size_t left, size_t right) {
    for (int i = left; i < right; i++) { // Инициализируем вектор x значениями 1 и массив res значениями 0.
        x[i] = 1;
        res[i] = 0;
    }

    for (int i = left * COUNT; i < right * COUNT; i++) { // Инициализируем матрицу M значениями 2.
        M[i] = 2;
    }
}

void matrixMult(size_t left, size_t right) { // Умножаем матрицу M на вектор x и сохраняем результат в массиве res.
    int half_sum = 0;

    for (int i = left; i < right; i++) { // определяем диапазон элементов матрицы, который будет обрабатываться текущим потоком.
        half_sum += M[i] * x[i % COUNT];

        if (i % COUNT == COUNT - 1) {// Когда индекс i достигает конца строки матрицы, half_sum добавляется к элементу вектора с использованием мьютекса для защиты разделяемого ресурса.
            mut.lock();
            res[i / COUNT] += half_sum;
            half_sum = 0;
            mut.unlock();
        }
    }

    if ((right - 1) % COUNT == COUNT - 1) { // Если последний элемент в диапазоне также завершает строку матрицы, остаточная частичная сумма добавляется к соответствующему элементу результирующего вектора.
        mut.lock();
        res[(right - 1)/ COUNT] += half_sum;
        mut.unlock();
    }
}

int main()
{
    x = new double[COUNT];
    res = new double[COUNT];
    M = new double[COUNT*COUNT];

    for (int j = 1; j <= 40; j++) { // создаем необходимое количество потоков для инициализации и умножения.
        int countThreads = j, left, right;
        int items_per_thread = COUNT / countThreads;
        int items_per_thread_mod = COUNT % countThreads;

        vector<thread> threads;

        auto start = std::chrono::steady_clock::now();
        for (int i = 0; i < countThreads; i++) {
            left = i * items_per_thread + (items_per_thread_mod > i? i: items_per_thread_mod);
            right = (i + 1) * items_per_thread + (items_per_thread_mod > i? i + 1: items_per_thread_mod);

            threads.push_back(thread(define, left, right));
        }

        for (auto &thread : threads) {
            thread.join();
        }

        threads.clear();
        items_per_thread = (COUNT * COUNT) / countThreads, items_per_thread_mod = (COUNT * COUNT) % countThreads;

        for (int i = 0; i < countThreads; i++) {
            left = i * items_per_thread + (items_per_thread_mod > i? i: items_per_thread_mod);
            right = (i + 1) * items_per_thread + (items_per_thread_mod > i? i + 1: items_per_thread_mod);
            threads.push_back(thread(matrixMult, left, right));
        }

        for (auto &thread : threads) {
            thread.join();
        }

        auto end = std::chrono::steady_clock::now();

        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        std::cout << "Time taken for step " << j << ": " << duration.count() << " milliseconds." << std::endl;

        threads.clear();
    }

    for (int i = 0; i < COUNT; i++) { // Вычисляем время выполнения каждого шага и выводит его на экран.
        cout << res[i] << ' ';
    }

    return 0;
}
