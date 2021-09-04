#include <vector>
#include <type_traits>
#include <set>
#include <memory>
#include <iostream>
#define LEN 8000000
#define BOUND 32


using std::shared_ptr;


template <size_t chunkSize>
struct FixedAllocator {
	static size_t bucketSize;
	static int* mem;
	static std::vector<int*> free;
	//static int* beg;
	FixedAllocator() {};
	int* add() {
		if (free.empty()) {
			mem += bucketSize;
			//if (mem - beg > static_cast<int>(LEN - bucketSize)) {
				//std::cerr << "no len\n";
			//}
			return mem;
		}
		else {
			int* to_return = free.back();
			free.pop_back();
			return to_return;
		}
	}
	void free_mem(int* ptr) {
		free.push_back(ptr);
	}
};
template <size_t chunkSize>
int* FixedAllocator<chunkSize>::mem = reinterpret_cast<int*>(malloc(sizeof(int) * LEN));
template <size_t chunkSize>
size_t FixedAllocator<chunkSize>::bucketSize = chunkSize / 4;
template <size_t chunkSize>
std::vector<int*> FixedAllocator<chunkSize>::free = std::vector<int*>();
//template <size_t chunkSize>
//int* FixedAllocator<chunkSize>::beg = FixedAllocator<chunkSize>::mem;


template <typename T>
struct FastAllocator {
	FastAllocator() = default;
	template <typename U>
	FastAllocator(const FastAllocator<U>&) {}
	template <typename U>
	FastAllocator& operator= (const FastAllocator<U>&) {
		return *this;
	}
	using value_type = T;
	template <typename U>
	struct rebind {
		using other = FastAllocator<U>;
	};
	T* allocate(size_t n) {
		if (sizeof(T) <= BOUND && n == 1) {     //n % 0 do i need it
			return reinterpret_cast<T*>(FixedAllocator<sizeof(T)>().add());
		}
		else {
			return reinterpret_cast<T*>(malloc(sizeof(T) * n));
		}
	}
	void deallocate(T* ptr, size_t n) {
		if (sizeof(T) > BOUND || n != 1) {
			free(ptr);
		}
		else {
			FixedAllocator<sizeof(T)>().free_mem(reinterpret_cast<int*>(ptr));
		}
	}
};


template <typename T, typename Allocator = std::allocator<T>>
class List {
private:
	struct Node {
		Node* prev = nullptr;
		Node* next = nullptr;
		T value;
		Node() = default;
		Node(Node* prev, Node* next, const T& value) : prev(prev), next(next), value(value) {};
		Node(Node* prev, Node* next) : prev(prev), next(next) {};
	};
	using alloc_t = typename std::allocator_traits<Allocator>::template rebind_alloc<Node>;
	alloc_t alloc;
	Node* root = nullptr;
	size_t sz = 0;
	void add_after(Node* x, const T& value) {
		Node* newnode = alloc.allocate(1);
		new(newnode) Node(x, x->next, value);
		x->next = x->next->prev = newnode;
		++sz;
	}
	void add_after_novalue() {
		Node* x = root->prev;
		Node* newnode = alloc.allocate(1);
		std::allocator_traits<alloc_t>::construct(alloc, newnode, x, x->next);
		x->next = x->next->prev = newnode;
		++sz;
	}
	void connect(Node* first, Node* second) {
		first->next = second;
		second->prev = first;
	}
	void erase_node(Node* x) {
		if (sz > 0) --sz;
		std::allocator_traits<alloc_t>::destroy(alloc, x);
		alloc.deallocate(x, 1);
	}
	void erase_root() {
		alloc.deallocate(root, 1);
	}
	void init_root() {
		root = alloc.allocate(1);
		root->next = root;
		root->prev = root;
	}
public:
	explicit List(const Allocator& alloc = Allocator()) : alloc(alloc) {
		init_root();
	};
	List(const List<T, Allocator>& x) {
		alloc = std::allocator_traits<alloc_t>::select_on_container_copy_construction(x.get_allocator());
		init_root();
		for (const_iterator it = x.cbegin(); it != x.cend(); ++it) {
			push_back(*it);
		}
	}
	List& operator= (const List<T, Allocator>& x) {
		if (std::allocator_traits<alloc_t>::propagate_on_container_copy_assignment::value) {
			alloc = x.get_allocator();
		}
		this->~List();
		sz = 0;
		init_root();
		for (const_iterator it = x.cbegin(); it != x.cend(); ++it) {
			push_back(*it);
		}
		return *this;
	}
	List(size_t count, const T& value, const Allocator& alloc = Allocator()) : alloc(alloc) {
		init_root();
		for (size_t i = 0; i < count; ++i) {
			push_back(value);
		}
	}
	List(size_t count, const Allocator& alloc = Allocator()) : alloc(alloc) {
		init_root();
		for (size_t i = 0; i < count; ++i) {
			add_after_novalue();
		}
	}
	~List() {
		Node* tmp = root->next;
		while (tmp != root) {
			tmp = tmp->next;
			erase_node(tmp->prev);
		}
		erase_root();
	}
	void push_back(const T& value) {
		add_after(root->prev, value);
	}
	void push_front(const T& value) {
		add_after(root, value);
	}
	void pop_back() {
		Node* to_delete = root->prev;
		connect(root->prev->prev, root);
		erase_node(to_delete);
	}
	void pop_front() {
		Node* to_delete = root->next;
		connect(root, root->next->next);
		erase_node(to_delete);
	}
	alloc_t get_allocator() const {
		return alloc;
	}
	size_t size() const {
		return sz;
	}

	template<bool IsConst>
	struct common_iterator {
		using iterator_category = std::bidirectional_iterator_tag;
		using value_type = std::conditional_t<IsConst, const T, T>;
		using difference_type = int;
		using pointer = value_type*;
		using reference = value_type&;
		Node* ptr;
		common_iterator(Node* ptr) : ptr(ptr) {}
		common_iterator& operator++() {
			ptr = ptr->next;
			return *this;
		}
		common_iterator& operator--() {
			ptr = ptr->prev;
			return *this;
		}
		common_iterator operator++(int) {
			common_iterator copy(*this);
			++(*this);
			return copy;
		}
		common_iterator operator--(int) {
			common_iterator copy(*this);
			--(*this);
			return copy;
		}
		reference operator  *() {
			return ptr->value;
		}
		pointer operator->() {
			return &(ptr->value);
		}
		operator common_iterator<true>() const {
			return common_iterator<true>(this->ptr);
		}
		template <bool Other>
		bool operator== (const common_iterator<Other>& other) const {
			return (ptr == other.ptr);
		}
		template <bool Other>
		bool operator!= (const common_iterator<Other>& other) const {
			return (ptr != other.ptr);
		}
	};
	using iterator = common_iterator<false>;
	using const_iterator = common_iterator<true>;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;

	void insert(const const_iterator& it, const T& value) {
		add_after(it.ptr->prev, value);
	}
	void erase(const const_iterator& it) {
		Node* to_delete = it.ptr;
		connect(it.ptr->prev, it.ptr->next);
		erase_node(to_delete);
	}
	iterator begin() {
		return iterator(root->next);
	}
	iterator end() {
		return iterator(root);
	}
	const_iterator begin() const {
		return const_iterator(root->next);
	}
	const_iterator end() const {
		return const_iterator(root);
	}
	const_iterator cbegin() const {
		return const_iterator(root->next);
	}
	const_iterator cend() const {
		return const_iterator(root);
	}
	reverse_iterator rbegin() {
		return reverse_iterator(root);
	}
	reverse_iterator rend() {
		return reverse_iterator(root->next);
	}
	const_reverse_iterator rbegin() const {
		return const_reverse_iterator(root);
	}
	const_reverse_iterator rend() const {
		return const_reverse_iterator(root->next);
	}
	const_reverse_iterator crbegin() const {
		return const_reverse_iterator(root);
	}
	const_reverse_iterator crend() const {
		return const_reverse_iterator(root->next);
	}
};