#pragma once

/////////////////////////////////////////////////////////////////////////////////////
// this is a "smart pointer" class that is designed to hide implementation classes
// and provide some automatic functions like: reference counting, copy on write.
// right now, copy on write is built in, there are times when i've modified this
// class to not perform copy on write. i suppose i should rework this class into
// a couple of new ones: rcimpl (no copy on write) & rcsharedimpl (has copy on
// write).
//
// to use this class your class must have both an assignment operator and copy
// constructor.

template <class T> class rcimpl { 
public:
	rcimpl(T* ptr);
	rcimpl(const rcimpl<T>& that);
	~rcimpl();

	const rcimpl<T>& operator = (T* that);
	const rcimpl<T>& operator = (const rcimpl<T>& that);

	int operator == (const rcimpl<T>& that) const;
	int operator != (const rcimpl<T>& that) const;

	T* operator -> ();
	const T* operator -> () const;

	T& operator * ();
	const T& operator * () const;

	unsigned int reference_count() const;

private:
	void copier(const rcimpl<T>* that);
	void release();

	struct rc_type {
		unsigned int rc;
		T* ptr;
	};
	rc_type* rep;
};


/////////////////////////////////////////////////////////////////////////////////////
// this is the implementation of a reference counting template; that is used for private
// date member representations. originally, pre vc++ rc_type class had some member functions 
// that handled the ptr/rc stuff but because of errors in these, i've removed the functions:
//
// template < class T >
// rcimpl<T>::rc_type::rc_type(T* px = 0) { rc = 1; ptr = px; }
//
// template < class T >
// rcimpl<T>::rc_type::~rc_type() { delete ptr; }
//
// and instead "spread" the functionality around to the rest of the member functions.
//

template < class T >
rcimpl<T>::rcimpl(T* ptr) {
	rep = new rc_type;
	rep->ptr = ptr;
	rep->rc = 1;
}

template < class T >
rcimpl<T>::rcimpl(const rcimpl<T>& that) {
	rep = 0;
	copier(&that);
}

template < class T>
rcimpl<T>::~rcimpl() {
	release();
}

template < class T >
const rcimpl<T>& rcimpl<T>::operator = (T* that) {
	release();
	rep = new rc_type;
	rep->ptr = that;
	rep->rc = 1;
	return *this;
}

template < class T >
const rcimpl<T>& rcimpl<T>::operator = (const rcimpl<T>& that) {
	if ( this != &that ) {
		copier(&that);
	}
	return *this;
}

template < class T >
int rcimpl<T>::operator == (const rcimpl<T>& that) const {
	if ( rep && that.rep ) {
		return (rep == that.rep);
	}
	return 0;
}

template < class T >
int rcimpl<T>::operator != (const rcimpl<T>& that) const {
	return !(*this == that);
}

template < class T >
T* rcimpl<T>::operator -> () {
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
const T* rcimpl<T>::operator -> () const {
	return rep->ptr;
}

template < class T >
T& rcimpl<T>::operator * () {
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
const T& rcimpl<T>::operator * () const {
	return *(rep->ptr);
}

template < class T >
unsigned int rcimpl<T>::reference_count() const {
	return (rep) ? rep->rc : 0;
}

template < class T >
void rcimpl<T>::copier(const rcimpl<T>* that) {
	if ( that ) that->rep->rc++;
	release();
	rep = that->rep;
}

template < class T >
void rcimpl<T>::release() {
	if ( rep && --rep->rc == 0 ) {
		delete rep->ptr;
		delete rep;
		rep = 0;
	}
}