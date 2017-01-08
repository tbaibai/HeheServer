#ifndef __DOUBLE_BUUFER_QUEUE_H__
#define __DOUBLE_BUUFER_QUEUE_H__
#include <atomic>
#include <mutex>
#include <vector>
#include <queue>
#include <functional>
//适用于一个线程生产，另一个线程消费的场景
//对于生产和消费者都不阻塞
//size用于限制队列的大小
//无锁
//使用时,push和isFull只能由生产者调用,pull和isEmpty只能由消费者调用
template<typename T>
class DoubleBufferQueue{
public:
	DoubleBufferQueue()
	: DoubleBufferQueue([](T& t){if(t) delete t;}){

	}

	DoubleBufferQueue(std::function<void(T& t)> delete_func)
	: delete_func_(delete_func){
		for (auto i = 0; i < 2; ++i){
			queues_.push_back(std::queue<T>());
			mutexes_.push_back(new std::mutex());
		}
	}

	~DoubleBufferQueue(){
		for (auto it = queues_.begin(); it != queues_.end(); ++it){
			while(!it->empty()){
				T& t = it->front();
				delete_func_(t);
				it->pop();
			}
		}
		for (auto it = mutexes_.begin(); it != mutexes_.end(); ++it){
			delete *it;
		}
	}

	void push(T& t){
		auto push_index = _nextIndex(pop_index_.load());
		auto& push_queue = queues_[push_index];
		auto& push_mutex = mutexes_[push_index];

		std::lock_guard lk(*push_mutex);
		push_queue.push(t);
	}

	bool pop(T* t){
		auto pop_index = pop_index_.load();
		auto& pop_queue = queues_[pop_index];
		auto& pop_mutex = mutexes_[pop_index];
		auto push_index = _nextIndex(pop_index);
		auto& push_queue = queues_[push_index];
		auto& push_mutex = mutexes_[push_index];

		std::lock_guard lk(*pop_mutex);
		if (pop_queue.empty()){
			std::lock_guard lk(*push_mutex);
			if (push_queue.empty()){
				*t = nullptr;
				return false;
			}
			else{
				_swap();
				*t = push_queue.front();
				push_queue.pop();
				return true;
			}
		}
		else{
			*t = pop_queue.front();
			pop_queue.pop();
			return true;
		}
	}

private:
	int _nextIndex(int v){
		return (v + 1) % 2;
	}

	void _swap(){
		auto current_index = pop_index_.load();
		auto push_index = _nextIndex(current_index);
		pop_index_.store(push_index);
	}
private:
	std::vector<std::queue<T>> queues_;
	std::vector<std::mutex*> mutexes_;
	std::atomic<int> pop_index_ = 0;
	std::function<void(T& t)> delete_func_;
};

#endif
