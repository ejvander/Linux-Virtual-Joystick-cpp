#ifndef _VJOY_PYTHON_H
#define _VJOY_PYTHON_H

#include "vjoy.h"


#ifdef PYTHON3
extern "C" PyMODINIT_FUNC PyInit_vjoy();
#else
extern "C" void initvjoy();
#endif

#endif /* _VJOY_PYTHON_H */
