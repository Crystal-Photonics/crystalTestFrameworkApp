#include "thread_pool.h"

#include <cassert>

struct Thread::Thread_base : QThread {
    Thread_base(std::function<void()> f)
        : function{std::move(f)} {
        start();
    }
    void run() override {
        function();
    }
    ~Thread_base() {
        quit();
    }
    std::function<void()> function;
};

Thread::Thread(Thread &&) = default;
Thread &Thread::operator=(Thread &&) = default;

Thread::Thread(std::function<void()> function)
    : thread_base{std::make_unique<Thread_base>(std::move(function))} {}

void Thread::join() {
    assert(thread_base);
    thread_base->wait();
}

bool Thread::is_finished() const {
    if (not thread_base) {
        return false;
    }
    return thread_base->isFinished();
}

Thread::~Thread() = default;

Thread_pool::Thread_pool(unsigned int threads)
    : workers(threads) {
    for (auto &worker : workers) {
        worker = Thread{[&] {
            for (;;) {
                std::function<void()> work;
                {
                    // get work from work queue
                    std::unique_lock l{worker_queue_mutex};
                    condition_variable.wait(l, [this] { return not work_queue.empty(); });
                    work = std::move(work_queue.front());
                    work_queue.pop_front();
                }
                if (not work) { //empty function means worker thread should quit
                    return;
                }
                work();
            }
        }};
    }
}

Thread_pool::~Thread_pool() {
    close_workers();
    for (auto &thread : workers) { //wait unti threads have finished
        thread.join();
    }
}

void Thread_pool::push(std::function<void()> f) {
    //add work to work queue
    assert(f); //don't allow users to push empty work which quits a thread
    std::lock_guard _(worker_queue_mutex);
    work_queue.push_back(std::move(f));
    condition_variable.notify_one();
}

void Thread_pool::close_workers() {
    { //tell each thread to quit
        std::lock_guard _(worker_queue_mutex);
        for ([[maybe_unused]] const auto &thread : workers) {
            work_queue.emplace_back(); //push empty function that quits a worker thread
        }
    }
    condition_variable.notify_all();
}

[[nodiscard]] bool Thread_pool::workers_closed() {
    while (not workers.empty()) {
        if (workers.back().thread_base->isFinished()) {
            workers.pop_back();
        } else {
            return false;
        }
    }
    return true;
}