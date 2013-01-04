#ifndef _VJOY_H
#define _VJOY_H

#include <linux/input.h>
#include <linux/uinput.h>

#include <cstring>
#include <cstdlib>

#include <vector>
#include <map>
#include <string>
#include <exception>

#include "device.h"
#include "python_include.h"

//error handling mechanism
class error: public std::exception {
private:
	std::string problem;

public:
	error(const std::string& message) throw () :
			problem(message) {
	}

	virtual ~error() throw () {
	}

	virtual const char* what() throw () {
		return problem.c_str();
	}
};

PyObject* convert_ff_effect(struct ff_effect* effect);
PyObject* convert_ff_envelope(struct ff_envelope *envelope);

class vjoy {
public:
	static pthread_mutex_t pymutex;

	static void init(const char* extraPath);

	static void end() {
		delete device::dev;
	}
};

#endif /* _VJOY_H */
