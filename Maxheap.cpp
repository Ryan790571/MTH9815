// Chaofan Shen 22/11/2022
#include <iostream>

template <typename T>
class MaxHeap
{
private:
	T** arr;
	int n;

public:
	MaxHeap(int size)
	{
		arr = new T * [size];
		n = 0;
	}

	void maxheap()
	{
		int k;
		for (k = n / 2; k >= 1; k--) {
			heapify(k, n);
		}
	}

	void heapify(int n, int i)
	{
		int left = 2 * i + 1;
		int right = 2 * i + 2;

		int smallest = i;
		if (left < n && arr[left] > arr[smallest])
			smallest = left;

		if (right < n && arr[right] > arr[smallest])
			smallest = right;

		if (smallest != i)
		{
			swap(*arr, i, smallest);
			heapify(n, smallest);
		}
	}

	void add(T key)
	{
		n = n + 1;
		arr[n - 1] = &key;
		maxheap();
	}

	T remove()
	{
		T* last_value = arr[n - 1];
		T top_value = *arr[0];
		arr[0] = last_value;
		n = n - 1;
		maxheap();
		return top_value;
	}

	void swap(T* arr, int i, int j)
	{
		T temp = arr[i];
		arr[i] = arr[j];
		arr[j] = temp;
	}
};

int main()
{
	// test the case
	MaxHeap<int> heap(5);
	heap.add(3);
	heap.add(5);
	heap.add(13);
	heap.add(20);
	heap.add(1);

	for (int i = 0; i < 5; i++)
	{
		std::cout << heap.remove() << ", ";
	}

	return 0;
}