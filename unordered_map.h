#include <vector>
#include <utility>
#include <exception>
#include <functional>
#define BEG_SIZE 1000
using std::pair;


template <typename T, typename Allocator = std::allocator<T>>
class List {
public:
	Allocator default_alloc = Allocator();
	struct Node {
		Node* prev = nullptr;
		Node* next = nullptr;
		T* value = nullptr;
		unsigned long long cached;
		Node() = default;
		template <typename U>
		Node(Allocator& default_alloc, Node* prev, Node* next, U&& x) : prev(prev), next(next) {
			value = default_alloc.allocate(1);
			std::allocator_traits<Allocator>::construct(default_alloc, value, std::forward<U>(x));
		};
		template <typename... Args>
		Node(Allocator& default_alloc, Node* prev, Node* next, Args&&... args) : prev(prev), next(next) {
			value = default_alloc.allocate(1);
			std::allocator_traits<Allocator>::construct(default_alloc, value, std::forward<Args>(args)...);
		};
		Node(Node* prev, Node* next) : prev(prev), next(next) {};
	};
	using alloc_t = typename std::allocator_traits<Allocator>::template rebind_alloc<Node>;
	alloc_t alloc;
	Node* root = nullptr;
	size_t sz = 0;
	template <typename U>
	void add_after(Node* x, U&& value) {
		Node* newnode = alloc.allocate(1);
		std::allocator_traits<alloc_t>::construct(alloc, newnode, default_alloc, x, x->next, std::forward<U>(value));
		x->next = x->next->prev = newnode;
		++sz;
	}
	void reconnect(Node* first, Node* second) {
		second->prev->next = second->next;
		second->next->prev = second->prev;
		second->prev = first;
		second->next = first->next;
		first->next->prev = second;
		first->next = second;
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
		std::allocator_traits<Allocator>::destroy(default_alloc, x->value);
		default_alloc.deallocate(x->value, 1);
		x->value = nullptr;
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
	//----------------------------------------------------------------------------------------------------
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
	List(List&& x) : sz(x.sz) {
		init_root();
		if (std::allocator_traits<alloc_t>::propagate_on_move_assignment::value) {
			alloc = std::move(x.alloc);
		}
		connect(root, x.root->next);
		connect(x.root->prev, root);
		x.sz = 0;
		connect(x.root, x.root);
	}
	List& operator=(List&& x) {
		this->~List();
		root = x.root;
		sz = x.sz;
		alloc = std::move(x.alloc);
		x.init_root();
		x.sz = 0;
		return *this;
	}
	List& operator=(const List<T, Allocator>& x) {
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
	void push_back(const T& value) {
		add_after(root->prev, value);
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
		size_t hash() const {
			return ptr->cached;
		}
		common_iterator plus_one() const {
			common_iterator copy(*this);
			++copy;
			return copy;
		}
		common_iterator minus_one() const {
			common_iterator copy(*this);
			--copy;
			return copy;
		}
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
			return *(ptr->value);
		}
		pointer operator->() {
			return ptr->value;
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
	void replace_by(Node* to, Node* from) {
		if (to != from) {
			from->prev = to->prev;
			from->next = to->next;
			erase_node(to);
		}
	}
	template <typename... Args>
	void emplace_front(Args&&... args) {
		Node* node = alloc.allocate(1);
		std::allocator_traits<alloc_t>::construct(alloc, node, default_alloc, root, root->next, std::forward<Args>(args)...);
		root->next = root->next->prev = node;
		++sz;
	}
};


template <typename Key, typename Value, typename Hash = std::hash<Key>, typename Equal = std::equal_to<Key>, typename Alloc = std::allocator<pair<const Key, Value>>>
class UnorderedMap {
public:
	double max_load;
	using NodeType = pair<const Key, Value>;
	List<NodeType, Alloc> data;
	using Node_ptr_t = typename List<NodeType, Alloc>::Node*;
	std::vector<Node_ptr_t> hash_table;
	UnorderedMap() {
		hash_table.resize(BEG_SIZE, nullptr);
		max_load = 10;
	}
	UnorderedMap(UnorderedMap&& x) : max_load(x.max_load_factor()) {
		if (this != &x) {
			data = std::move(x.data);
			hash_table = std::move(x.hash_table);
			x.max_load = 10;            
		}
	}
	UnorderedMap(const UnorderedMap& x) : max_load(x.max_load_factor()) {
		data = x.data;
		hash_table = x.hash_table;
	}
	UnorderedMap& operator=(const UnorderedMap& x) {
		max_load = x.max_load_factor();
		data = x.data;
		hash_table = x.hash_table;
		return *this;
	}
	UnorderedMap& operator=(UnorderedMap&& x) {
		if (this != &x) {
			max_load = x.max_load;
			data = std::move(x.data);
			hash_table = std::move(x.hash_table);
			x.max_load = 10;              
		}
		return *this;
	}
	~UnorderedMap() {};   
	//-----------------------------------------
	//		methods
	//-----------------------------------------
	Value& at(const Key& x) {
		unsigned long long index = Hash{}(x) % hash_table.capacity();
		Node_ptr_t node = hash_table[index];
		if (node == nullptr) {
			throw "no such key";
		}
		while (node->cached == index) {
			if (Equal()(node->value->first, x)) {
				return node->value->second;
			}
			node = node->next;
		}
		throw "no such key";
	}
	size_t size() const {
		return data.size();
	}
	double load_factor() const {
		return size() / static_cast<double>(hash_table.size());
	}
	double max_load_factor() const {
		return max_load;
	}
	void max_load_factor(double x) {
		max_load = x;
		if (load_factor() > max_load) rehash(size() / max_load);
	}
	size_t max_size() {
		return hash_table.size() * max_load_factor;
	}
	void reserve(size_t count) {
		if (count > hash_table.size() * max_load) hash_table.resize(count / max_load);
	}
	Value& operator[] (const Key& x) {
		unsigned long long index = Hash{}(x) % hash_table.capacity();
		Node_ptr_t node = hash_table[index];
		if (node == nullptr) {
			data.add_after(data.root, std::make_pair(x, Value()));
			data.root->next->cached = index;
			hash_table[index] = data.root->next;
			if (load_factor() > max_load) {
				rehash(2 * hash_table.size());
			}
			return data.root->next->value->second;
		}
		while (node->cached == index) {
			if (Equal()(node->value->first, x)) {
				return node->value->second;
			}
			node = node->next;
		}
		data.add_after(node->prev, std::make_pair(x, Value()));
		node->prev->cached = index;
		if (load_factor() > max_load) {
			rehash(2 * hash_table.size());
		}
		return node->prev->value->second;
	}
	//-----------------------------------------
	//		iterators
	//-----------------------------------------
	using iterator = typename List<NodeType, Alloc>::iterator;
	using const_iterator = typename List<NodeType, Alloc>::const_iterator;
	iterator begin() {
		return data.begin();
	}
	iterator end() {
		return data.end();
	}
	const_iterator begin() const {
		return data.cbegin();
	}
	const_iterator end() const {
		return data.cend();
	}
	const_iterator cbegin() const {
		return data.cbegin();
	}
	const_iterator cend() const {
		return data.cend();
	}
	void erase(const const_iterator& it) {
		if (it.plus_one().hash() != it.hash() && it.minus_one().hash() != it.hash()) {
			hash_table[it.hash()] = nullptr;
		}
		data.erase(it);
	}
	void erase(const_iterator first, const const_iterator& second) {
		if (first == second) return;
		auto to_erase = first++;
		while (first != second) {
			erase(to_erase);
			to_erase = first;
			++first;
		}
		erase(to_erase);
	}
	iterator find(const Key& x) {
		unsigned long long index = Hash{}(x) % hash_table.capacity();
		Node_ptr_t node = hash_table[index];
		if (node == nullptr) {
			return end();
		}
		while (node->cached == index) {
			if (Equal()(node->value->first, x)) {
				return iterator(node);
			}
			node = node->next;
		}
		return end();
	}
	const_iterator find(const Key& x) const {
		unsigned long long index = Hash{}(x) % hash_table.capacity();
		Node_ptr_t node = hash_table[index];
		if (node == nullptr) {
			return cend();
		}
		while (node->cached == index) {
			if (Equal()(node->value->first, x)) {
				return const_iterator(node);
			}
			node = node->next;
		}
		return cend();
	}
	void rehash(size_t count) {
		if (count < size() / max_load_factor()) count = size() / max_load_factor();
		hash_table.assign(count, nullptr);
		List<NodeType, Alloc> newdata;
		size_t index;
		Node_ptr_t node = nullptr;
		for (auto it = begin(); it != end(); it = begin()) {
			index = Hash{}(it.ptr->value->first) % hash_table.capacity();
			if (hash_table[index] == nullptr) {
				node = newdata.root;
			}
			else {
				node = hash_table[index];
			}
			data.reconnect(node, it.ptr);
			hash_table[index] = it.ptr;
			++newdata.sz;
			--data.sz;
		}
		data = std::move(newdata);
	}
	pair<iterator, bool> insert(const NodeType& x) {
		unsigned long long index = Hash{}(x.first) % hash_table.capacity();
		Node_ptr_t node = hash_table[index];
		if (node == nullptr) {
			data.add_after(data.root, x);
			data.root->next->cached = index;
			hash_table[index] = data.root->next;
			if (load_factor() > max_load) {
				rehash(2 * hash_table.size());
			}
			return std::make_pair(iterator(data.root->next), true);
		}
		while (node->cached == index && node != data.root) {
			if (Equal()(node->value->first, x.first)) {
				return std::make_pair(iterator(node), false);
			}
			node = node->next;
		}
		data.add_after(node->prev, x);
		node->prev->cached = index;
		if (load_factor() > max_load) {
			rehash(2 * hash_table.size());
		}
		return std::make_pair(iterator(node->prev), true);
	}
	template <typename U>
	pair<iterator, bool> insert(U&& x) {
		unsigned long long index = Hash{}(x.first) % hash_table.capacity();
		Node_ptr_t node = hash_table[index];
		if (node == nullptr) {
			data.add_after(data.root, std::forward<U>(x));
			data.root->next->cached = index;
			hash_table[index] = data.root->next;
			if (load_factor() > max_load) {
				rehash(2 * hash_table.size());
			}
			return std::make_pair(iterator(data.root->next), true);
		}
		while (node->cached == index && node != data.root) {
			if (Equal()(node->value->first, x.first)) {
				return std::make_pair(iterator(node), false);
			}
			node = node->next;
		}
		data.add_after(node->prev, std::forward<U>(x));
		node->prev->cached = index;
		if (load_factor() > max_load) {
			rehash(2 * hash_table.size());
		}
		return std::make_pair(iterator(node->prev), true);
	}
	//-----------------------------------------------------------------
	template <typename InputIt>
	void insert(InputIt left, InputIt right) {   //WHAT
		for (; left != right; ++left) {
			insert(*left);
		}
	}
	template <typename... Args>
	pair<iterator, bool> emplace(Args&&... args) {
		data.emplace_front(std::forward<Args>(args)...);                       ?
		unsigned long long index = Hash{}(data.root->next->value->first) % hash_table.capacity();
		Node_ptr_t node = hash_table[index];
		if (node == nullptr) {
			data.root->next->cached = index;
			hash_table[index] = data.root->next;
			if (load_factor() > max_load) {
				rehash(2 * hash_table.size());
			}
			return std::make_pair(iterator(data.root->next), true);
		}
		while (node->cached == index) {
			if (Equal()(node->value->first, data.root->next->value->first)) {
				data.replace_by(node, data.root->next);
				return std::make_pair(iterator(node), false);
			}
			node = node->next;
		}
		data.reconnect(node->prev, data.root->next);
		node->prev->cached = index;
		if (load_factor() > max_load) {
			rehash(2 * hash_table.size());
		}
		return std::make_pair(iterator(node->prev), true);
	}
};