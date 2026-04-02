#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <QThread>
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

struct Thread {
    template <class F, class Arg, class... Args>
    Thread(F f, Arg arg, Args... args)
        : Thread{std::make_unique<QThread>([function = std::move(f), arguments = std::tuple{std::move(arg), std::move(args)...}] {
            call(function, arguments, std::index_sequence_for<Arg, Args...>());
        })} {}

    Thread() = default;
    Thread(Thread &&);
    Thread &operator=(Thread &&);
    Thread(std::function<void()> function);
    ~Thread();

    void join();
    [[nodiscard]] bool is_finished() const;

    template <class Function, class Tuple, std::size_t... indexes>
    static auto call(Function &&function, Tuple &tuple, std::index_sequence<indexes...>) {
        return std::invoke(function, std::get<indexes>(tuple)...);
    }

    struct Thread_base;
    std::unique_ptr<Thread_base> thread_base;
};

struct Thread_pool {
    Thread_pool(unsigned int threads = std::thread::hardware_concurrency());
    Thread_pool(const Thread_pool &) = delete;
    Thread_pool &operator=(const Thread_pool &) = delete;
    void close_workers();
    [[nodiscard]] bool workers_closed();
    ~Thread_pool();
    void push(std::function<void()> f);

    private:
    std::mutex worker_queue_mutex;
    std::deque<std::function<void()>> work_queue;
    std::condition_variable condition_variable;
    std::vector<Thread> workers;
};

#endif // THREAD_POOL_H