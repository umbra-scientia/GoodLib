#include <atomic>
struct RCObj {
	RCObj();
	virtual ~RCObj();
	void Reserve();
	void Release();
	virtual bool Valid();
	std::atomic<int> usage;
};
template <class T> struct Ref {
	T* ptr;
	Ref() : ptr(0) {}
	Ref(T* p, bool skip_reserve = false) : ptr(p) { if (ptr && !skip_reserve) ptr->Reserve(); }
	Ref(const Ref<T>& r) : ptr(r.ptr) { if (ptr) ptr->Reserve(); }
	~Ref() { if (ptr) ptr->Release(); }
	Ref<T>& operator=(T* p) {
		if (p) p->Reserve();
		if (ptr) ptr->Release();
		ptr = p;
		return *this;
	}
	Ref<T>& operator=(const Ref<T>& r) {
		T* p = r.ptr;
		if (p) p->Reserve();
		if (ptr) ptr->Release();
		ptr = p;
		return *this;
	}
	template <class U> Ref<U> Cast() { return Ref<U>(dynamic_cast<U*>(ptr)); }
	operator bool() const { return ptr; }
	bool operator!() const { return !ptr; }
	bool operator<(const Ref<T>& rhs) const { return ptr < rhs.ptr; }
	T* operator->() const { return ptr; }
	const T& operator*() const { return *ptr; }
	T& operator*() { return *ptr; }
	operator const T*() const { return ptr; }
	operator T*() { return ptr; }
};
