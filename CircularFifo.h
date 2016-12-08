#ifndef __CIRCULAR_FIFO_H__
#define __CIRCULAR_FIFO_H__
#include <atomic>
//适用于一个线程生产，另一个线程消费的场景
//对于生产和消费者都不阻塞
//size用于限制队列的大小
//无锁
//使用时,push和isFull只能由生产者调用,pull和isEmpty只能由消费者调用
template<typename Element, size_t size>
class CircularFifo{
public:
	CircularFifo() : tail_(0), head_(0){}

	bool push(const Element& item){
		auto current_tail = tail_.load();
		auto next_tail = _increment(current_tail);
		if(next_tail != head_.load())
		{
			array_[current_tail] = item;
			tail_.store(next_tail);
			return true;
		}

		return false;//队列满
	}
	bool pull(Element& item){
		auto current_head = head_.load();
		if(current_head != tail_.load()){
			item = array_[current_head];
			head_.store(_increment(current_head));
			return true;
		}

		return false;//队列空
	}
	bool isEmpty(){
		return (head_.load() == tail_.load());
	}
	bool isFull(){
		auto next_tail = _increment(tail_.load());
		return (next_tail == head_.load());
	}
private:
	size_t _increment(size_t idx){
		return (idx + 1) % (size + 1);
	}
private:
	Element array_[size + 1];
	std::atomic <size_t>  tail_;
	std::atomic <size_t>  head_;
};

#endif
