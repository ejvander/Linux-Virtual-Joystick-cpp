#ifndef PYTHON_INCLUDE_H
#define PYTHON_INCLUDE_H

#ifdef PYTHON3
#include <python3.2/Python.h>
#else
#include <python2.7/Python.h>
#endif
#include <pthread.h>

#ifndef PYTHON3
#define PyLong_AsLong PyInt_AsLong //ints become longs in python 3
#endif

#endif /* PYTHON_INCLUDE_H_ */
