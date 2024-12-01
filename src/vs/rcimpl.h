#pragma once

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

template <class T>
rcimpl<T>::rcimpl(T* ptr) {
    rep = new rc_type;
    rep->rc = 1;
    rep->ptr = ptr;
}

template <class T>
rcimpl<T>::rcimpl(const rcimpl<T>& that) {
    copier(&that);
}

template <class T>
rcimpl<T>::~rcimpl() {
    release();
}

template <class T>
const rcimpl<T>& rcimpl<T>::operator = (T* that) {
    if (rep->ptr != that) {
        release();
        rep = new rc_type;
        rep->rc = 1;
        rep->ptr = that;
    }
    return *this;
}

template <class T>
const rcimpl<T>& rcimpl<T>::operator = (const rcimpl<T>& that) {
    if (this != &that) {
        release();
        copier(&that);
    }
    return *this;
}

template <class T>
int rcimpl<T>::operator == (const rcimpl<T>& that) const {
    return rep->ptr == that.rep->ptr;
}

template <class T>
int rcimpl<T>::operator != (const rcimpl<T>& that) const {
    return rep->ptr != that.rep->ptr;
}

template <class T>
T* rcimpl<T>::operator -> () {
    return rep->ptr;
}

template <class T>
const T* rcimpl<T>::operator -> () const {
    return rep->ptr;
}

template <class T>
T& rcimpl<T>::operator * () {
    return *(rep->ptr);
}

template <class T>
const T& rcimpl<T>::operator * () const {
    return *(rep->ptr);
}

template <class T>
unsigned int rcimpl<T>::reference_count() const {
    return rep->rc;
}

template <class T>
void rcimpl<T>::copier(const rcimpl<T>* that) {
    rep = that->rep;
    ++(rep->rc);
}

template <class T>
void rcimpl<T>::release() {
    if (--(rep->rc) == 0) {
        delete rep->ptr;
        delete rep;
    }
}
