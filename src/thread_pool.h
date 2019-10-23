#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <cassert>
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

struct Thread_pool {
	Thread_pool(unsigned int threads = std::thread::hardware_concurrency())
		: workers(threads) {
		// initialize worker threads
		for (auto &thread : workers) {
			thread = std::thread{[this] {
				for (;;) {
					std::function<void()> work;
					{ // get work from work queue
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
	Thread_pool(const Thread_pool &) = delete;
	Thread_pool &operator=(const Thread_pool &) = delete;
	~Thread_pool() {
		{ //tell each thread to quit
			std::unique_lock l(worker_queue_mutex);
			for (const auto &thread : workers) {
				work_queue.emplace_back(); //push empty function that quits a worker thread
			}
		}
		condition_variable.notify_all();
		for (auto &thread : workers) { //wait unti threads have finished
			thread.join();
		}
	}

	void push(std::function<void()> f) {
		//add work to work queue
		assert(f); //don't allow users to push empty work which quits a thread
		std::unique_lock l(worker_queue_mutex);
		work_queue.push_back(std::move(f));
		condition_variable.notify_one();
	}

	private:
	std::mutex worker_queue_mutex;
	std::deque<std::function<void()>> work_queue;
	std::vector<std::thread> workers;
	std::condition_variable condition_variable;
};
#endif // THREAD_POOL_H
