#ifndef _VJOY_H
#define _VJOY_H

#include <linux/input.h>
#include <linux/uinput.h>

#include <cstring>
#include <cstdlib>

#include <vector>
#include <string>
#include <exception>

#include "module.h"
#include "python_include.h"

//error handling mechanism
class error: public std::exception {
private:
	std::string problem;

public:
	error(const std::string& message) throw () {
		problem = message;
	}

	virtual ~error() throw () {

	}

	virtual const char* what() throw () {
		return problem.c_str();
	}
};

PyObject* vjoy_convert_ff_effect(struct ff_effect* effect);
PyObject* vjoy_convert_ff_envelope(struct ff_envelope *envelope);

class vjoy {
public:
	static std::vector<char*> searchPaths;
	static std::vector<module*> modules;

	static pthread_mutex_t pymutex;

	static void addSearchPath(const char* path) {
		int len = strlen(path);
		char* c = (char*) malloc(len);
		strcpy(c, path);

		searchPaths.push_back(c);
	}

	static void loadModules() {
		for (std::vector<module*>::iterator it = modules.begin(); it != modules.end(); it++)
			(*it)->load();
	}

	static void init();

	static void end() {
		for (std::vector<module*>::iterator it = modules.begin(); it != modules.end(); it++)
			delete *it;

		for (std::vector<char*>::iterator it = searchPaths.begin(); it != searchPaths.end(); it++)
			delete *it;
	}
};

#endif /* _VJOY_H */
