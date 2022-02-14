/*
 * Copyright 2000, Brown University, Providence, RI.
 * 
 *                         All Rights Reserved
 * 
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose other than its incorporation into a
 * commercial product is hereby granted without fee, provided that the
 * above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Brown University not be used in
 * advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.
 * 
 * BROWN UNIVERSITY DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR ANY
 * PARTICULAR PURPOSE.  IN NO EVENT SHALL BROWN UNIVERSITY BE LIABLE FOR
 * ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#ifndef GLUE_LIST_H
#define GLUE_LIST_H

#include <stdlib.h>  // for qsort


extern "C" {
typedef int (* compare_func_t) (const void *, const void *);
}

static const int BAD_IND = -999;

//----------------------------------------------
//
//  ARRAY:
//      Templated array-based list. It resizes
//  itself as necessary. Constructor lets you
//  specify the expected size.
//
//----------------------------------------------
#define cARRAY const ARRAY
template <class T>
class ARRAY {
 protected:
   T      *_array;      // pointer to the data
   int     _num;        // number of elements in the array
   int     _max;        // max elements for currently allocated array
   int     _unique;

   // needed by LIST (below -- ARRAY of REFptrs): 
   virtual void clear_ele  (int)        {}
   virtual void clear_range(int, int)   {}
   void  append_ele(const T& el) {
         // append element
         if (_num == _max) realloc();
         _array[_num++] = el;
   }

 public:
   // ******** MANAGERS ********
   ARRAY(int m=0) : _array(0), _num(0), _max(m > 0 ? m : 0), _unique(0) {
      if (_max) _array = new T[_max];
   }
   ARRAY(cARRAY<T>& l) : _array(0), _num(0), _max(0), _unique(0) { *this = l; }
   ARRAY(const T &e) : _array(0), _num(0), _max(0), _unique(0) { ARRAY<T> l;
                                                                 l += e;
								 *this = l; }
   virtual ~ARRAY() { clear(); delete [] _array;}

   // ******** ACCESSORS/CONVENIENCE ********
   int          num()              const { return _num; }
   bool         empty()            const { return (_num<=0); }
   bool         valid_index(int k) const { return (k>=0 && k<_num); }
   void         set_unique()             { _unique = 1; }

   const T*     array()            const { return _array; }
   T*           array()                  { return _array; }
   T&           operator [](int j)       { return _array[j]; }
   T&           last()                   { if (empty())
                                              printf("ARRAY::last-list empty");
                                           return _array[_num-1]; }
   const T& operator [](int j)     const { return _array[j]; }
   const T&           last()       const { if (empty())
                                              printf("ARRAY::last-list empty");
                                           return _array[_num-1]; }

   // ******** MEMORY MANAGEMENT ********

   void clear() { clear_range(0,_num); _num=0; }
   virtual void truncate(int n) {
      if (valid_index(n)) {
         clear_range(n,_num);
         _num = n;
      } else printf("ARRAY::truncate: bad index %d", n);
   }

   // realloc() is called automatically as necessary
   // when the array runs out of room, or explicitly
   // when it's known that the array will shortly need
   // to contain a given number of elements. new_max
   // tells how large the array should be -- if the
   // array is already that large nothing happens.
   // otherwise, the array is reallocated and its elements
   // are copied to the new array. if new_max == 0, this
   // is interpreted as a request that _max should be
   // doubled. the exception is when _max is also 0
   // (meaning the array itself is null), in which case
   // _max is set to 1 and the array is allocated to
   // hold a single element.
   virtual void realloc(int new_max=0) {
       if (new_max && new_max <= _max)
          return;
       _max = (new_max == 0) ? (_max ? _max*2 : 1) : new_max;
       T *tmp = new T [_max];
       if (tmp == 0)
          printf("ARRAY: realloc failed");
       for (int i=0; i<_num; i++) {
          tmp[i] = _array[i];
          clear_ele(i);
       }
       delete [] _array;
       _array = tmp;
   }

   // ******** CONTAINMENT ********
   int get_index(const T &el) const {
      // return index of element
      // or BAD_IND if element is not found:
      for (int k = _num-1; k >= 0; k--)
         if (_array[k] == el)
            return k;
      return BAD_IND;
   }
   bool contains(const T &el) const { return get_index(el) != BAD_IND; }

   // ******** ADDING ********
   void operator += (const T& el) {
      if (_unique)
          add_uniquely(el);
      else
          append_ele(el);
   }

   // append (same as +=):
   void add (const T& p) { *this += p; }     

   // add to front:
   void push(const T& p) {
      insert(0,1);
      _array[0] = p;
   } 
   
   bool add_uniquely(const T& el) {
      // add element only if it isn't already there:
      int ret=0;
      if (get_index(el) == BAD_IND) {
         append_ele(el);
         ret = 1;
      }
      return ret;
   }

   // XXX - is this used??
   void insert(int ind, int num) {
      // open up a gap of uninitialized elements
      // in the array, starting at index 'ind',
      // extending for 'num' elements.
      // 
      // presumably these elements then 
      // get assigned directly.
      if (_num+num >= _max) 
         realloc(_max+num);
      _num += num;
      for (int i = _num-1; i>=ind+num; i--)
         _array[i] = _array[i-num];
   }
   
   void insertAt(const T &el, int index) {
	   insert(index, 1);	// Make a gap at 'index' for 1 element
	   _array[index] = el;	// And put it there
   }

   // ******** REMOVING ********
   bool remove(int k) {
      // remove element k
      // return 1 on success, 0 on failure:
      if (valid_index(k)) {
         // replace element k with last element and shorten list:
         _array[k] = _array[--_num];
         clear_ele(_num);
         return 1;
      } else if (k != BAD_IND) {
         printf("ARRAY::remove: invalid index %d", k);
         return 0;
      } else return 0; // assume the call to get_index() failed
   }
   
   bool remove(int k, bool keep_order) {
		// remove element k
		// return 1 on success, 0 on failure:
		if (valid_index(k)) {
			if (keep_order) {
				for (int i = k; i<_num-1; i++) {
					_array[i] = _array[i+1];
				}
				clear_ele(--_num);
				return 1;
			}
			else {
				// replace element k with last element and shorten list:
				return remove(k);
			}
	
		} else if (k != BAD_IND) {
			printf("ARRAY::remove: invalid index %d", k);
			return 0;
		} else return 0; // assume the call to get_index() failed
   }
   
   // search for given element, remove it:
   void operator -= (cARRAY<T> &l)        { for (int i=0; i < l.num(); i++)
                                               *this -= l[i]; }

   bool operator -= (const T &el)         { return remove(get_index(el)); }
   bool          rem(const T &p)          { return (*this -= p); }
                
   T pop() {
      // delete last element in list and return it:
      T tmp = last();
      remove(_num-1);
      return tmp;
   }

   // ******** ARRAY OPERATORS ********
   ARRAY<T>& operator =(cARRAY<T>& l) {
      // assignment operator:
      if (&l == this)  // don't do anything if rhs already is lhs
         return *this;
      clear();
      if(!l.empty()) {
         realloc(l._num);
         for (int i=0; i<l._num; i++)
            *this += l[i];
      }
      return *this;
   }
   ARRAY<T>& operator +=(cARRAY<T>& l) {
      // concatenation operator:
      if(!l.empty()) {
         realloc(_num + l._num);
         for (int i=0; i<l._num; i++)
            *this += l[i];
      }
      return *this;
   }
   bool operator ==(const ARRAY<T> &c) const {
       if (num() != c.num())
          return 0;
       for (int i = 0; i < num(); i++) {
          // Use !(x == y) because == should be available and != may not be
          if (!((*this)[i] == c[i]))
             return 0;
       }
       return 1;
   }

   // ******** REORDERING ********
   void reverse() {
      for (int i=0, j=_num-1; i<j; ++i, --j) {
         // swap causes potential conflicts, so we do this instead...
         T tmp     = _array[i];
         _array[i] = _array[j];
         _array[j] = tmp;
      }
   }
   virtual void sort(compare_func_t compare) {
      qsort(_array, _num, sizeof(T), compare);
   }
};



//----------------------------------------------
//
//  LIST:
//
//      same as ARRAY, but assumes the templated
//      type T is derived from a REFptr, and calls
//      Clear() on ref pointers as needed.
//
//----------------------------------------------
#define cLIST const LIST
template <class T>
class LIST : public ARRAY<T> {
 protected:
   virtual void clear_ele  (int i)          { ARRAY<T>::_array[i].Clear(); }
   virtual void clear_range(int i, int j)   { for (int k=i; k < j; k++)
                                                 clear_ele(k); }
 public:
   LIST(int m=0)     : ARRAY<T>(m) {}
   LIST(cLIST<T>& l) : ARRAY<T>(l) {}
};



//----------------------------------------------
//
//  OBSlist<OBS>:
//
//      Templated list of observers.  This makes it
//      easy to create a class for a list of a specific
//      type of observer.   Given that list class, 
//      typically, you will create a single global
//      instance that registers all observers and
//      dispatches events to the observers.
//
//----------------------------------------------
/*template <class OBS>
class OBSlist 
{
   protected:
      ARRAY<OBS *> _list;
   public:
      virtual ~OBSlist() {}
               OBSlist() : _list(0) {} // Start out with empty list
      int  num()             const { return _list.num(); }
      void obs(OBS *o)             { _list.add_uniquely(o); }
      void obs_first(OBS *o)       { if (_list.empty()) _list.add(o);
                                     else {
                                       _list.insert(0, 1);
                                       _list[0] = o;
                                    } }
      void unobs(OBS *o)           { _list.rem(o); }

//      void notify(const GLUE_TYPENAME OBS::data &d) {
//                                     for (int i=0; i < _list.num(); i++)
//                                        _list[i]->notify(d); }
     
};


template<class T>
ostream &
operator<< (ostream &os, const ARRAY<T> &array)
{
   for (int i = 0; i < array.num(); i++)  {
       os << array[i];
       if (array.num() > 1) os << endl;
   }
   return os;
}

template<class T>
istream &
operator>> (istream &is, ARRAY<T> &array)
{
   array.clear();
   WHITESPACE(is); // skip over extra whitespace
   while (!is.eof() && is.peek() != '\0' && is.peek() != '}') {
      T x; is >> x; array += x;
      WHITESPACE(is); // skip over extra whitespace
   }
   return is;
}*/

#endif  // list.H

/* end of file list.H.H */
