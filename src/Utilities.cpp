#include "Utilities.hpp"

#if ENABLE_MULTITHREADED

ThreadedImageSaver::ThreadedImageSaver() {
	for (size_t i = 0; i < std::thread::hardware_concurrency(); i++) {
		threads.emplace_back(&ThreadedImageSaver::runThread, this);
	}
}

ThreadedImageSaver::~ThreadedImageSaver() {
	stopped = true;
	(void)std::lock_guard<std::mutex>(mtx);
	cv.notify_all();
	for (auto& thread : threads) {
		thread.join();
	}
}

void ThreadedImageSaver::runThread() {
	std::unique_lock<std::mutex> l(mtx);
	while (!stopped) {
		cv.wait(l);
		while (!images.empty()) {
			auto img = std::move(images.back());
			images.pop_back();
			l.unlock();
			img.first.writePNG(img.second);
			l.lock();
		}
	}
}

void ThreadedImageSaver::enqueue(Image img, fs::path path) {
	{
		std::lock_guard<std::mutex> l(mtx);
		images.emplace_back(std::move(img), std::move(path));
	}
	cv.notify_one();
}

#endif
