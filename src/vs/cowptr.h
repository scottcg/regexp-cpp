
#ifndef INCLUDE_cow_auto_ptr_H
#define INCLUDE_cow_auto_ptr_H

/////////////////////////////////////////////////////////////////////////////////////
// this is a "smart pointer" class that is designed to hide implementation classes
// and provide some automatic functions like: reference counting, copy on write.
// right now, copy on write is built in, there are times when i've modified this
// class to not perform copy on write. i suppose i should rework this class into
// a couple of new ones: cow_auto_ptr (no copy on write) & rcsharedimpl (has copy on
// write).
//
// to use this class your class must have both an assignment operator and copy
// constructor.

template < class T > class cow_auto_ptr {
public:
	typedef T element_type;

public:
	cow_auto_ptr(T* ptr);
	cow_auto_ptr(const cow_auto_ptr<T>& rhs);
	~cow_auto_ptr();

	const cow_auto_ptr<T>& operator = (element_type* rhs);
	const cow_auto_ptr<T>& operator = (const cow_auto_ptr<T>& rhs);

	int operator == (const cow_auto_ptr<T>& rhs) const;
	int operator != (const cow_auto_ptr<T>& rhs) const;

	T* operator -> ();
	const T* operator -> () const;
	
	T& operator * ();
	const T& operator * () const;

private:
	void copier(const cow_auto_ptr<T>* rhs);
	void release();
	unsigned int reference_count() const;

	struct rc_type {
		unsigned int	rc;
		element_type*	ptr;
	};
	rc_type* rep;
};


/////////////////////////////////////////////////////////////////////////////////////
// this is the implementation of a reference counting template; rhs is used for private
// date member representations. originally, pre vc++ rc_type class had some member functions 
// that handled the ptr/rc stuff but because of errors in these, i've removed the functions:
//
// template < class T >
// cow_auto_ptr<T>::rc_type::rc_type(T* px = 0) { rc = 1; ptr = px; }
//
// template < class T >
// cow_auto_ptr<T>::rc_type::~rc_type() { delete ptr; }
//
// and instead "spread" the functionality around to the rest of the member functions.
//

template < class T >
cow_auto_ptr<T>::cow_auto_ptr(T* ptr) {
	rep = new rc_type;
	rep->ptr = ptr;
	rep->rc = 1;
}

template < class T >
cow_auto_ptr<T>::cow_auto_ptr(const cow_auto_ptr<T>& rhs) {
	rep = 0;
	copier(&rhs);
}

template < class T>
cow_auto_ptr<T>::~cow_auto_ptr() {
	release();
}

template < class T >
const cow_auto_ptr<T>& cow_auto_ptr<T>::operator = (T* rhs) {
	release();
	rep = new rc_type;
	rep->ptr = rhs;
	rep->rc = 1;
	return *this;
}

template < class T >
const cow_auto_ptr<T>& cow_auto_ptr<T>::operator = (const cow_auto_ptr<T>& rhs) {
	if ( this != &rhs ) {
		copier(&rhs);
	}
	return *this;
}

template < class T >
int cow_auto_ptr<T>::operator == (const cow_auto_ptr<T>& rhs) const {
	if ( rep && rhs.rep ) {
		return (rep == rhs.rep);
	}
	return 0;
}

template < class T >
int cow_auto_ptr<T>::operator != (const cow_auto_ptr<T>& rhs) const {
	return !(*this == rhs);
}

template < class T >
T* cow_auto_ptr<T>::operator -> () {
	if ( rep->rc > 1 ) {	// copy on write
		const T* temp_ptr = rep->ptr;
		rep->rc--;
		rep = new rc_type;
		rep->rc = 1;
		rep->ptr = new T(*temp_ptr);
	}
	return rep->ptr;
}

template < class T >
const T* cow_auto_ptr<T>::operator -> () const {
	return rep->ptr;
}

template < class T >
T& cow_auto_ptr<T>::operator * () {
	if ( rep->rc > 1 ) {	// copy on write
		const T* temp_ptr = rep->ptr;
		rep->rc--;
		rep = new rc_type;
		rep->rc = 1;
		rep->ptr = new T(*temp_ptr);
	}
	return *(rep->ptr);
}

template < class T >
const T& cow_auto_ptr<T>::operator * () const {
	return *(rep->ptr);
}

#if vc5
template < class T >
cow_auto_ptr<T>::operator const T* () const {
	return (rep) ? rep->ptr : 0;
}
#endif

template < class T >
unsigned int cow_auto_ptr<T>::reference_count() const {
	return (rep) ? rep->rc : 0;
}

template < class T >
void cow_auto_ptr<T>::copier(const cow_auto_ptr<T>* rhs) {
	if ( rhs ) rhs->rep->rc++;
	release();
	rep = rhs->rep;
}

template < class T >
void cow_auto_ptr<T>::release() {
	if ( rep && --rep->rc == 0 ) {
		delete rep->ptr;
		delete rep;
		rep = 0;
	}
}

#endif
