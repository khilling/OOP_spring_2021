#include <vector>
#include <memory>
#include <cstring>
#include <iterator>
#include <type_traits>
#include "stdlib.h"
#include <stdexcept>


using std::shared_ptr;


template<typename T>
struct Deque {
	static const size_t len = 10;
	size_t sz = 0;
	size_t shift = 0;
	size_t number_of_buckets = 1;
	shared_ptr<T[]>* arr = reinterpret_cast<shared_ptr<T[]>*>(new char[sizeof(shared_ptr<T[]>)]);
	template<bool IsConst>
	struct common_iterator {
		using iterator_category = std::bidirectional_iterator_tag;
		using value_type = std::conditional_t<IsConst, const T, T>;
		using difference_type = int;
		using pointer = value_type*;
		using reference = value_type&;
		shared_ptr<T[]>* ptr;
		size_t index;
		common_iterator<IsConst>(shared_ptr<T[]>* ptr, size_t index) : ptr(ptr), index(index) {}
		common_iterator<IsConst>& operator++() {
			++index;
			return *this;
		}
		common_iterator<IsConst>& operator--() {
			--index;
			return *this;
		}
		common_iterator<IsConst>& operator+(const size_t n) {
			index += n;
			return *this;
		}
		common_iterator<IsConst>& operator-(const size_t n) {
			index -= n;
			return *this;
		}
		int operator-(const common_iterator<true>& other) {
			return index - other.index;
		}
		reference operator  *() {
			return ptr[index / len][index % len];
		}
		pointer operator->() {
			return &(ptr[index / len][index % len]);
		}
		operator common_iterator<true>() const {
			return common_iterator<true>(this->ptr, this->index);
		}
		template <bool Other>
		bool operator== (const common_iterator<Other>& other) const {
			return (index == other.index);    //check ptr??
		}
		template <bool Other>
		bool operator!= (const common_iterator<Other>& other) const {
			return (index != other.index);
		}
		template <bool Other>
		bool operator> (const common_iterator<Other>& other) const {
			return index > other.index;
		}
		template <bool Other>
		bool operator< (const common_iterator<Other>& other) const {
			return index < other.index;
		}
		template <bool Other>
		bool operator>= (const common_iterator<Other>& other) const {
			return (*this > other) || (other == *this);
		}
		template <bool Other>
		bool operator<= (const common_iterator<Other>& other) const {
			return (*this < other) || (other == *this);
		}
	};
	using iterator = common_iterator<false>;
	using const_iterator = common_iterator<true>;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;
	Deque<T>() {
		try {
			new(arr) shared_ptr<T[]>(reinterpret_cast<T*>(new char[len * sizeof(T)]));
		}
		catch (...) {
			arr[0].~shared_ptr<T[]>();
			throw;
		}
	}
	Deque<T>(const Deque<T>& x) : sz(x.sz) {
		shared_ptr<T[]>* save = arr;
		size_t i = 0;
		try {
			number_of_buckets = sz == 0 ? 0 : (sz - 1) / len + 1;
			arr = reinterpret_cast<shared_ptr<T[]>*>(new char[number_of_buckets * sizeof(shared_ptr<T[]>)]);
			for (; i < number_of_buckets; ++i) {
				new(arr + i) shared_ptr<T[]>(reinterpret_cast<T*>(new char[len * sizeof(T)]));
			}
		}
		catch (...) {
			for (size_t j = 0; j < i; ++j) {
				arr[j].~shared_ptr<T[]>();
			}
			delete[] arr;
			arr = save;
			sz = 0;
			number_of_buckets = 1;
			throw;
		}
		for (size_t j = 0; j < sz; ++j) {
			new(&arr[j / len][j % len]) T(x[j]);
		}
	}
	Deque<T>(const int& n) : sz(n) {
		number_of_buckets = sz == 0 ? 0 : (sz - 1) / len + 1;
		shared_ptr<T[]>* save = arr;
		try {
			arr = reinterpret_cast<shared_ptr<T[]>*>(new char[number_of_buckets * sizeof(shared_ptr<T[]>)]);
		}
		catch (...) {
			number_of_buckets = 1;
			sz = 0;
			delete[] arr;
			arr = save;
			throw;
		}
		size_t i = 0;
		try {
			for (; i < number_of_buckets; ++i) {
				new(arr + i) shared_ptr<T[]>(reinterpret_cast<T*>(new char[len * sizeof(T)]));
			}
		}
		catch (...) {
			number_of_buckets = 1;
			sz = 0;
			for (size_t j = 0; j < i; ++j) {
				arr[j].~shared_ptr<T[]>();
			}
			delete[] arr;
			arr = save;
			throw;
		}
	}
	Deque<T>(const int& n, const T& x) : sz(n) {
		size_t save_num = number_of_buckets;
		size_t i = 0;
		shared_ptr<T[]>* save = arr;
		number_of_buckets = sz == 0 ? 0 : (sz - 1) / len + 1;
		try {
			arr = reinterpret_cast<shared_ptr<T[]>*>(new char[number_of_buckets * sizeof(shared_ptr<T[]>)]);
			for (; i < number_of_buckets; ++i) {
				new(arr + i) shared_ptr<T[]>(reinterpret_cast<T*>(new char[len * sizeof(T)]));
			}
		}
		catch (...) {
			number_of_buckets = save_num;
			for (size_t j = 0; j < i; ++j) {
				arr[j].~shared_ptr<T[]>();
			}
			delete[] arr;
			arr = save;
			throw;
		}
		for (size_t i = 0; i < sz; ++i) {
			new(&arr[i / len][i % len]) T(x);
		}
	}
	Deque<T>& operator= (const Deque<T>& x) {
		size_t save_sz = sz;
		size_t save_shift = shift;
		size_t save_num = number_of_buckets;
		shift = 0;
		sz = x.sz;
		number_of_buckets = sz == 0 ? 0 : (sz - 1) / len + 1;
		shared_ptr<T[]>* newarr = arr;
		try {
			newarr = reinterpret_cast<shared_ptr<T[]>*>(new char[number_of_buckets * sizeof(shared_ptr<T[]>)]);
			for (size_t i = 0; i < number_of_buckets; ++i) {
				new(newarr + i) shared_ptr<T[]>(reinterpret_cast<T*>(new char[len * sizeof(T)]));
			}
			for (size_t i = 0; i < sz; ++i) {
				new(&newarr[i / len][i % len]) T(x[i]);
			}
			clear();
			arr = newarr;
			return *this;
		}
		catch (...) {
			delete[] newarr;
			sz = save_sz;
			shift = save_shift;
			number_of_buckets = save_num;
			throw;
		}
	}
	void clear() {
		for (size_t i = shift; i < sz + shift; ++i) {
			(&arr[i / len][i % len])->~T();
		}
		delete[] reinterpret_cast<char*>(arr);
	}
	~Deque<T>() {
		clear();
	}
	size_t size() const {
		return sz;
	}
	T& operator[] (const size_t& i) {
		size_t real_i = shift + i;
		return arr[real_i / len][real_i % len];
	}
	const T& operator[] (const size_t& i) const {
		size_t real_i = shift + i;
		return arr[real_i / len][real_i % len];
	}
	void expand_left(shared_ptr<T[]>* newarr) {
		size_t i = 0;
		try {
			for (; i < number_of_buckets; ++i) {
				new(newarr + i) shared_ptr<T[]>(reinterpret_cast<T*>(new char[len * sizeof(T)]));
			}
		}
		catch (...) {
			for (size_t j = 0; j < i; ++j) {
				newarr[j].~shared_ptr<T[]>();
			}
			throw;
		}
	}
	void move_centre(shared_ptr<T[]>* newarr) {
		for (size_t i = 0; i < number_of_buckets; ++i) {
			new(newarr + number_of_buckets + i) shared_ptr<T[]>(arr[i]);
		}
	}
	void expand_right(shared_ptr<T[]>* newarr) {
		size_t i = 2 * number_of_buckets;
		try {
			for (; i < 3 * number_of_buckets; ++i) {
				new(newarr + i) shared_ptr<T[]>(reinterpret_cast<T*>(new char[len * sizeof(T)]));
			}
		}
		catch (...) {
			for (size_t j = 2 * number_of_buckets; j < i; ++j) {
				newarr[j].~shared_ptr<T[]>();
			}
			throw;
		}
	}
	void expand() {
		shared_ptr<T[]>* newarr = reinterpret_cast<shared_ptr<T[]>*>(new char[3 * number_of_buckets * sizeof(shared_ptr<T[]>)]);
		expand_left(newarr);
		move_centre(newarr);
		expand_right(newarr);
		for (size_t i = 0; i < number_of_buckets; ++i) {
			(arr + i)->~shared_ptr<T[]>();
		}
		arr = newarr;
		shift += number_of_buckets * len;
		number_of_buckets *= 3;
	}
	void push_back(T x) {
		if (shift + sz == len * number_of_buckets) expand();
		new(&arr[(sz + shift) / len][(sz + shift) % len]) T(x);
		++sz;
	}
	void push_front(T x) {
		if (shift == 0) expand();
		--shift;
		new(&arr[shift / len][shift % len]) T(x);
		++sz;
	}
	void pop_back() {
		(&((*this)[sz - 1]))->~T();
		--sz;
	}
	void pop_front() {
		(&((*this)[0]))->~T();
		++shift;
		--sz;
	}
	iterator begin() {
		return iterator(arr, shift);
	}
	iterator end() {
		return iterator(arr, shift + sz);
	}
	const_iterator begin() const {
		return const_iterator(arr, shift);
	}
	const_iterator end() const {
		return const_iterator(arr, shift + sz);
	}
	const_iterator cbegin() const {
		return const_iterator(arr, shift);
	}
	const_iterator cend() const {
		return const_iterator(arr, shift + sz);
	}
	reverse_iterator rbegin() {
		return reverse_iterator(--end());
	}
	reverse_iterator rend() {
		return reverse_iterator(--begin());
	}
	const_reverse_iterator rbegin() const {
		return const_reverse_iterator(--end());
	}
	const_reverse_iterator rend() const {
		return const_reverse_iterator(--begin());
	}
	const_reverse_iterator crbegin() const {
		return const_reverse_iterator(--cend());
	}
	const_reverse_iterator crend() const {
		return const_reverse_iterator(--cbegin());
	}
	void insert(iterator it, const T& x) {
		if (sz == 0) {
			push_back(x);
			return;
		}
		push_back((*this)[sz - 1]);
		for (int i = 0; i < std::distance(it, end()) - 1; ++i) {
			(*this)[sz - 1 - i] = (*this)[sz - 2 - i]; //                         +-1 ?
		}
		*it = x;
	}
	void erase(iterator it) {
		for (size_t i = std::distance(it, end()); i < sz; ++i) {
			(*this)[i - 1] = (*this)[i]; //                         +-1 ?
		}
		pop_back();
	}
	T& at(size_t i) {
		if (i >= sz) {
			throw std::out_of_range("out of range");
		}
		return (*this)[i];
	}
};
