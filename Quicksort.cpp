// Chaofan Shen
// 11/22/2022

#include <iostream>
#include <string>


// Class for Quicksort
template <typename T>
class Quicksort
{
public:

	// overload () to start quick sort
	void operator() (T* arr, const int& n)
	{
		quick_sort(arr, 0, n - 1);
	}


	void quick_sort(T* arr, const int& low, const int& high)
	{
		if (low < high)
		{
			int pivot_index = partition(arr, low, high);
			quick_sort(arr, low, pivot_index - 1);
			quick_sort(arr, pivot_index + 1, high);
		}
	}


	int partition(T* arr, const int& low, const int& high)
	{
		int pivot = arr[high];
		int i = (low - 1);

		for (int j = low; j < high; j++)
		{
			if (arr[j] < pivot)
			{
				i++;
				swap(arr, i, j);
			}
		}
		swap(arr, i + 1, high);
		return (i + 1);
	}


	void swap(T* arr, const int& i, const int& j)
	{
		T temp = arr[i];
		arr[i] = arr[j];
		arr[j] = temp;
	}
};



int main()
{
	// test the cases
	int arr[] = { 15, 39, 21, 35, 3, 76, 8, 11 };
	int n = 8;
	Quicksort<int> qsort = Quicksort<int>();
	qsort(arr, n);

	for (int i = 0; i < n; ++i)
	{
		std::cout << arr[i] << ", ";
	}
	
	return 0;
}