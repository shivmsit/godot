#ifndef THREAD_WORK_POOL_H
#define THREAD_WORK_POOL_H

#include "core/os/memory.h"
#include "core/os/semaphore.h"
#include <atomic>
#include <thread>
class ThreadWorkPool {

	std::atomic<uint32_t> index;

	struct BaseWork {
		std::atomic<uint32_t> *index;
		uint32_t max_elements;
		virtual void work() = 0;
	};

	template <class C, class M, class U>
	struct Work : public BaseWork {
		C *instance;
		M method;
		U userdata;
		virtual void work() {

			while (true) {
				uint32_t work_index = index->fetch_add(1, std::memory_order_relaxed);
				if (work_index >= max_elements) {
					break;
				}
				(instance->*method)(work_index, userdata);
			}
		}
	};

	struct ThreadData {
		std::thread *thread;
		Semaphore start;
		Semaphore completed;
		std::atomic<bool> exit;
		BaseWork *work;
	};

	ThreadData *threads = nullptr;
	uint32_t thread_count = 0;

	static void _thread_function(ThreadData *p_thread);

public:
	template <class C, class M, class U>
	void do_work(uint32_t p_elements, C *p_instance, M p_method, U p_userdata) {

		ERR_FAIL_COND(!threads); //never initialized

		index.store(0);

		Work<C, M, U> *w = memnew((Work<C, M, U>));
		w->instance = p_instance;
		w->userdata = p_userdata;
		w->method = p_method;
		w->index = &index;
		w->max_elements = p_elements;

		for (uint32_t i = 0; i < thread_count; i++) {
			threads[i].work = w;
			threads[i].start.post();
		}
		for (uint32_t i = 0; i < thread_count; i++) {
			threads[i].completed.wait();
			threads[i].work = nullptr;
		}
	}

	void init(int p_thread_count = -1);
	void finish();
	~ThreadWorkPool();
};

#endif // THREAD_POOL_H
