#include "module.h"
#include "python_include.h"
#include "vjoy.h"

#include <fcntl.h>
#include <cassert>
#include <iostream>
#include <sstream>

using namespace std;

static void vjoy_parse_block(PyObject* info, char* key, int* count, int* array, int max) {
	// FIXME: Replace all these assert() with _real_ error handling
	PyObject *items = PyMapping_GetItemString(info, key);

	if (items) {
		*count = PySequence_Size(items);
		assert(*count >= 0);
		assert(*count <= max);

		for (int i = 0; i < *count; i++) {
			PyObject *item = PySequence_GetItem(items, i);
			assert(item != NULL);

			array[i] = PyLong_AsLong(item);
			assert(array[i] >= 0);
			Py_DECREF(item);
		}

		Py_DECREF(items);
	}
}

void module::parseInfo() {
	memset(&devinfo, 0, sizeof(vjoy_info));

	pthread_mutex_lock(&vjoy::pymutex);

	// Get info
	PyObject* pyinfo = PyObject_CallMethod(pymodule, const_cast<char*>("getVJoyInfo"), NULL);
	if (PyErr_Occurred() != NULL) {
		PyErr_Print();
	}

	if (!pyinfo) {
		cerr << "Module has no getVJoyInfo() method." << endl;
		return;
	}

	// Joystick name
	PyObject *pyname = PyMapping_GetItemString(pyinfo, const_cast<char*>("name"));
	if (pyname) {
		char* name = NULL;
#ifdef PYTHON3
		int size;
		wchar_t* _name = PyUnicode_AsWideCharString(pyname, &size);
		wcstombs(name, _name, size);
		PyMem_Free(_name);

#else
		name = PyString_AsString(pyname);
#endif

		if (name) {
			// Two names... This is kind of redundant.
			strncpy(devinfo.name, name, UINPUT_MAX_NAME_SIZE);
			strncpy(uidev.name, name, UINPUT_MAX_NAME_SIZE);
		}

		Py_DECREF(pyname);
	}

	// Relative axises
	vjoy_parse_block(pyinfo, const_cast<char*>("relaxis"), &devinfo.relaxiscount, devinfo.relaxis, REL_CNT);

	// Absolute axises
	vjoy_parse_block(pyinfo, const_cast<char*>("absaxis"), &devinfo.absaxiscount, devinfo.absaxis, ABS_CNT);

	// Force Feedback effects
	vjoy_parse_block(pyinfo, const_cast<char*>("feedback"), &devinfo.feedbackcount, devinfo.feedback, FF_CNT);

	PyObject* pymaxeffects = PyMapping_GetItemString(pyinfo, const_cast<char*>("maxeffects"));

	if (pymaxeffects) {
		devinfo.maxeffects = PyLong_AsLong(pymaxeffects);
		Py_DECREF(pymaxeffects);
	}

	// Buttons and keys
	vjoy_parse_block(pyinfo, const_cast<char*>("buttons"), &devinfo.buttoncount, devinfo.buttons, KEY_CNT);

	Py_DECREF(pyinfo);

	pthread_mutex_unlock(&vjoy::pymutex);
}

void module::load() {
	if (delay == 0) setFrequency(DEFUALT_INPUT_RATE);

	// Create device
	cout << "Initializing module: " << name << endl;

	// Start up Python
	cout << "\tImporting python module." << endl << endl;
	pthread_mutex_lock(&vjoy::pymutex);

	pymodule = PyImport_ImportModule(name.c_str());

	if (PyErr_Occurred()) {
		PyErr_Print();

		throw(error("Python error in module"));
	}

	if (!pymodule) {
		pthread_mutex_unlock(&vjoy::pymutex);
		throw(error(string("Failed to load module ") + name));
	}

	pthread_mutex_unlock(&vjoy::pymutex);

	// Open a connection to UInput
	for (std::vector<char*>::iterator it = vjoy::searchPaths.begin(); it != vjoy::searchPaths.end(); it++) {
		cout << "\t\tTrying " << *it << endl;
		uifd = open(*it, O_RDWR);
		if (uifd >= 0) break;
	}

	if (uifd < 0) throw(error("\tFailed to initialize uinput.\nAre you sure you have read/write permission to these paths?\n(Try running the program with sudo)"));

	cout << "\t\tSuccess!" << endl << endl;

	// Read device info from the Python module
	parseInfo();

	// Configure uinput device
	uidev.id.bustype = BUS_VIRTUAL;

	if (devinfo.relaxiscount > 0) {
		ioctl(uifd, UI_SET_EVBIT, EV_REL);
		for (int i = 0; i < devinfo.relaxiscount; i++) {
			cout << "\tAdding relative axis: " << devinfo.relaxis[i] << endl;
			ioctl(uifd, UI_SET_RELBIT, devinfo.relaxis[i]);
		}
	}
	if (devinfo.absaxiscount > 0) {
		ioctl(uifd, UI_SET_EVBIT, EV_ABS);
		for (int i = 0; i < devinfo.absaxiscount; i++) {
			cout << "\tAdding absolute axis: " << devinfo.absaxis[i] << endl;
			ioctl(uifd, UI_SET_ABSBIT, devinfo.absaxis[i]);
		}
	}
	if (devinfo.feedbackcount > 0) {
		ioctl(uifd, UI_SET_EVBIT, EV_FF);
		for (int i = 0; i < devinfo.feedbackcount; i++) {
			cout << "\tAdding feedback effect: " << devinfo.feedback[i] << endl;
			ioctl(uifd, UI_SET_FFBIT, devinfo.feedback[i]);
		}
	}
	if (devinfo.buttoncount > 0) {
		ioctl(uifd, UI_SET_EVBIT, EV_KEY);
		for (int i = 0; i < devinfo.buttoncount; i++) {
			cout << "\tAdding key/button: " << devinfo.buttons[i] << endl;
			ioctl(uifd, UI_SET_KEYBIT, devinfo.buttons[i]);
		}
	}

	for (int i = 0; i < ABS_MAX; i++) {
		uidev.absmin[i] = SHRT_MIN;
		uidev.absmax[i] = SHRT_MAX;
	}

	cout << "\tMax concurrent effects: " << devinfo.maxeffects << endl;
	uidev.ff_effects_max = devinfo.maxeffects;

	if (write(uifd, &uidev, sizeof(uinput_user_dev) != sizeof(uinput_user_dev))) throw(error("could not write device info"));

	int err = ioctl(uifd, UI_DEV_CREATE);
	stringstream s;
	s << "Device creation failed (" << err << ").";
	if (err) throw(s.str());

	cout << "Device created." << endl;
	cout << "Starting device control threads." << endl;
	cout << endl;

	pthread_create(&inputThread, NULL, module::inputLoop, this);
	pthread_create(&eventThread, NULL, module::eventLoop, this);
}

