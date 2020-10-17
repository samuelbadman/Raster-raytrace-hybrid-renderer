#pragma once

template <class T>
class Queue
{
public:
	Queue(int max)
	: m_maxPending(max)
	{
		m_queue = new T[m_maxPending];
	}

	~Queue()
	{
		delete[] m_queue;
	}

	void Push(const T& item)
	{
		//assert((m_tail + 1) % m_maxPending != m_head);
		// check if queue is full to reject item
		if ((m_tail + 1) % m_maxPending == m_head) return;
		m_queue[m_tail] = item;
		m_tail = (m_tail + 1) % m_maxPending;
	}

	const T& PopNextItem()
	{
		assert(m_head != m_tail);
		int head = m_head;
		m_head = (m_head + 1) % m_maxPending;
		return m_queue[head];
	}

	int GetMaxPending() const { return m_maxPending; }
	int GetHead() const { return m_head; }
	int GetTail() const { return m_tail; }
	int SizeInBytes() const { return sizeof(T) * m_maxPending; }

	// Walk over a queue.
	//for (int i = head_; i != tail_; i = (i + 1) % max_pending)

private:
	int m_maxPending;
	T* m_queue;
	int m_head = 0;
	int m_tail = 0;
};

