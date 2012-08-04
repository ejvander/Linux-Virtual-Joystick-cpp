#ifndef _VJOY_PYTHON_H
#define _VJOY_PYTHON_H

#include "vjoy.h"

#ifdef PYTHON3
PyMODINIT_FUNC PyInit_vjoy();
#else
void initvjoy();
#endif

void vjoy_py_initialize();

#endif /* _VJOY_PYTHON_H */
