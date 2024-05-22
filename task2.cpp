#include <iostream>
#include <future>
#include <thread>
#include <math.h>
#include <queue>
#include <unordered_map>
#include <numbers>

#define Type double

using namespace std;

condition_variable condVar, condVar2;

mutex mut;

queue<pair<size_t, future<Type>>> tasks;
queue<pair<size_t, future<Type>>> get_tasks;

unordered_map<int, Type> results;

void server_thread(const stop_token& stoken)
{
    unique_lock lock_res{mut, defer_lock};
    size_t id_task;
    future<Type> task;

    while (!stoken.stop_requested()) // Пока не запрошено завершение работы сервера, поток обрабатывает задачи.
    {
        lock_res.lock();

        if (tasks.empty()) { // Если очередь пуста, сервер ожидает сигнала от условной переменной condVar, чтобы продолжить выполнение.
            condVar.wait_for(lock_res, chrono::seconds(1s));
        }

        if (!tasks.empty()) { // Если очередь задач не пуста, первая задача перемещается в очередь
            get_tasks.push({tasks.front().first, std::move(tasks.front().second)});

            tasks.pop();
        }

        lock_res.unlock();

        while (!get_tasks.empty()) { // Обрабатываются все задачи в get_tasks: результаты выполнения задач сохраняются в контейнер results, и условная переменная condVar2 уведомляет клиентов о готовности результатов.
            id_task = get_tasks.front().first;
            Type result = get_tasks.front().second.get();

            lock_res.lock();

            results.insert({id_task, result});

            get_tasks.pop();

            condVar2.notify_all();

            lock_res.unlock();
        }
    }

    cout << "Server stop!\n";
}

template<typename T>
class Server{
public:
    void start() {
        cout << "Start\n";
        server = jthread(server_thread);
    };

    void stop() {
        server.request_stop();
        server.join();

        cout << "End\n";
    };

    size_t add_task(future<T> task) { // добавляет задачу в очередь задач, присваивая ей уникальный идентификатор
        size_t id = rand;
        tasks.push({id, std::move(task)});
        rand++;

        return id;
    };

    T request_result(size_t id_res) { // возвращает результат выполнения задачи по её идентификатору и удаляет её из контейнера результатов
        T res = results[id_res];

        results.erase(id_res);

        return res;
    };

private:
    size_t rand = 0;
    jthread server;
};

Server<Type> server;

template<typename T>
T fun_sin(T arg) {
    return sin(arg);
}

template<typename T>
T fun_sqrt(T arg) {
    return sqrt(arg);
}

template<typename T>
T fun_pow(T x, T y) {
    return pow(x, y);
}

template<typename T>
void add_task_1() {
    unique_lock lock_res(mut, defer_lock);

    for (int i = 0; i < 10000; i++) {
        future<T> result = async(launch::deferred, [](){return fun_sin<T>(numbers::pi/2);});

        lock_res.lock();

        size_t id = server.add_task(std::move(result)); // Клиент создаёт задачу на вычисление синуса и добавляет её на сервер

        condVar.notify_one();

        lock_res.unlock();

        lock_res.lock();

        while (results.find(id) == results.end()) {
            condVar2.wait(lock_res); // Клиент ожидает завершения задачи и получения результата

            if (results.find(id) != results.end()) {
                cout << "task_thread result:\t" << server.request_result(id) << endl;

                break;
            }
        }

        lock_res.unlock();
    }

    cout << "Client 1 work is done" << endl;
}

template<typename T>
void add_task_2() {
    unique_lock lock_res(mut, defer_lock);

    for (int i = 0; i < 10000; i++) {
        future<T> result = async(launch::deferred, [](){return fun_sqrt<T>(4);});

        lock_res.lock();

        size_t id = server.add_task(std::move(result));

        condVar.notify_one();

        lock_res.unlock();

        lock_res.lock();

        while (results.find(id) == results.end()) {
            condVar2.wait(lock_res);

            if (results.find(id) != results.end()) {
                cout << "task_thread result:\t" << server.request_result(id) << endl;

                break;
            }
        }

        lock_res.unlock();
    }

    cout << "Client 2 work is done" << endl;
}

template<typename T>
void add_task_3() {
    unique_lock lock_res(mut, defer_lock);

    for (int i = 0; i < 10000; i++) {
        future<T> result = async(launch::deferred, [](){return fun_pow<T>(2, 3);});

        lock_res.lock();

        size_t id = server.add_task(std::move(result));

        condVar.notify_one();

        lock_res.unlock();

        lock_res.lock();

        while (results.find(id) == results.end()) {
            condVar2.wait(lock_res);

            if (results.find(id) != results.end()) {
                cout << "task_thread result:\t" << server.request_result(id) << endl;

                break;
            }
        }

        lock_res.unlock();
    }

    cout << "Client 3 work is done" << endl;
}

int main() {
    server.start();

    thread task_1(add_task_1<Type>);
    thread task_2(add_task_2<Type>);
    thread task_3(add_task_3<Type>);

    task_1.join();
    task_2.join();
    task_3.join();

    server.stop();
}