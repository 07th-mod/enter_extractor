#pragma once

#include "Config.hpp"
#if ENABLE_MULTITHREADED
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <vector>
#endif

#ifdef __clang__
#  define ARTIFICIAL __attribute__((artificial))
#else
#  define ARTIFICIAL
#endif

static size_t align(size_t value, size_t alignment) {
	return (value + alignment - 1) / alignment * alignment;
}

template <typename MakeState, typename Execute>
void parallel_for(size_t begin, size_t end, MakeState makeState, Execute&& fn) {
#if ENABLE_MULTITHREADED
	std::atomic<size_t> i{begin};
	std::vector<std::thread> threads;
	for (int j = 0; j < std::thread::hardware_concurrency(); j++) {
		threads.emplace_back([&]() ARTIFICIAL {
			auto state = makeState();
			while (true) {
				size_t value = i.fetch_add(1, std::memory_order_relaxed);
				if (value >= end) { return; }
				fn(state, value);
			}
		});
	}
	for (auto& thread : threads) { thread.join(); }
#else
	auto state = makeState();
	for (size_t i = begin; i < end; i++) {
		fn(state, i);
	}
#endif
}

#if ENABLE_MULTITHREADED
#include "Image.hpp"
class ThreadedImageSaver {
	std::vector<std::thread> threads;
	std::vector<std::pair<Image, fs::path>> images;
	std::condition_variable cv;
	std::mutex mtx;
	bool stopped = false;
	void runThread();
public:
	void enqueue(Image img, fs::path path);
	ThreadedImageSaver();
	~ThreadedImageSaver();
};
#endif
