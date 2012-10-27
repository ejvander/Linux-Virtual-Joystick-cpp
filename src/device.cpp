#include "device.h"
#include "python_include.h"
#include "vjoy.h"

#include <cassert>
#include <errno.h>

#include <fcntl.h>
#include <libudev.h>

#include <iostream>
#include <sstream>

using namespace std;

device* device::dev = NULL;
pthread_mutex_t device::devMutex;

static void vjoy_parse_block(PyObject* info, char* key, int* count, int* array, int max) {
	PyObject* items = PyMapping_GetItemString(info, key);

	if (items) {
		*count = PySequence_Size(items);
		if (*count > max) cerr << "looks like there are more items in the " << key << " than allowed (" << max << ")" << endl;

		for (int i = 0; i < *count; i++) {
			PyObject* item = PySequence_GetItem(items, i);
			assert(item);

			array[i] = PyLong_AsLong(item);
			assert(array[i] >= 0);
			Py_DECREF(item);
		}

		Py_DECREF(items);
	}
}

void device::parseInfo() {
	memset(&devinfo, 0, sizeof(vjoy_info));

	pthread_mutex_lock(&vjoy::pymutex);

	//check for callback functions
	PyObject* dict = PyModule_GetDict(pymodule);

	pause_callback = PyDict_GetItemString(dict, "on_pause");
	resume_callback = PyDict_GetItemString(dict, "on_resume");
	kill_callback = PyDict_GetItemString(dict, "on_kill");

	// Get info
	PyObject* pyinfo = PyObject_CallMethod(pymodule, const_cast<char*>("getVJoyInfo"), NULL);
	if (PyErr_Occurred()) {
		PyErr_Print();
		throw(error("python error"));
	}

	if (!pyinfo) {
		err() << "Module has no getVJoyInfo() method." << endl;
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

	PyObject* pyEnableFF = PyMapping_GetItemString(pyinfo, const_cast<char*>("enable_ff"));

	if (pyEnableFF){
		enableFF = PyObject_IsTrue(pyEnableFF);
		Py_DECREF(pyEnableFF);
	}

	// Buttons and keys
	vjoy_parse_block(pyinfo, const_cast<char*>("buttons"), &devinfo.buttoncount, devinfo.buttons, KEY_CNT);

	Py_DECREF(pyinfo);

	pthread_mutex_unlock(&vjoy::pymutex);
}

void device::load() {
	// Create device
	cout << "Initializing device: " << name << ", updated every " << delay / 1000 << " milliseconds" << endl;

	// Start up Python
	cout << "\tImporting python module." << endl << endl;
	pthread_mutex_lock(&vjoy::pymutex);

	pymodule = PyImport_ImportModule(name.c_str());

	if (PyErr_Occurred()) {
		PyErr_Print();
		pthread_mutex_unlock(&vjoy::pymutex);
		throw(error("Python error in module"));
	}

	if (!pymodule) {
		pthread_mutex_unlock(&vjoy::pymutex);
		throw(error(string("Failed to load device ") + name));
	}

	//add a constant with the device name
	PyModule_AddStringConstant(pymodule, "device_name", const_cast<const char*>(name.c_str()));

	//add a suicide method to the module
	PyMethodDef killMethodDef =
		{ "kill_self", device::moduleKillSelf, METH_VARARGS, "kill this virtual joystick" };

	PyObject* killFunction = PyCFunction_New(&killMethodDef, NULL);
	PyMethod_New(killFunction, NULL, pymodule);
	//Py_DECREF(killFunction);

	pthread_mutex_unlock(&vjoy::pymutex);

	//create a virtual input device file path
	udev* _udev;
	udev_device* udev_dev;
	const char* devnode;
	int orig_errno;

	_udev = udev_new();

	if (!_udev) {
		cerr << "errno " << errno << " - " << strerror(errno) << endl;
		throw(error("could create a udev"));
	}

	udev_dev = udev_device_new_from_subsystem_sysname(_udev, "misc", "uinput");
	if (!udev_dev) {
		cerr << "errno " << errno << " - " << strerror(errno) << endl;
		throw(error("could create a udev_device"));
	}

	devnode = udev_device_get_devnode(udev_dev);
	if (!devnode) {
		cerr << "errno " << errno << " - " << strerror(errno) << endl;
		throw(error("could not get devnoce from udev_dev"));
	}

	devPath = (char*) malloc(strlen(devnode) + 1);

	strcpy(devPath, devnode);

	udev_device_unref(udev_dev);
	udev_unref(_udev);

	//open the device file
	cout << "\tInitializing UInput" << endl;

	//this read is non-blocking
	uifd = open(devPath, O_RDWR | O_NONBLOCK);
	if (uifd == -1) {
		cerr << "errno " << errno << " - " << strerror(errno) << endl;
		throw(error("could not open uinput file"));
	}

	cout << "\t\tSuccess! (file descriptor is " << uifd << ")" << endl << endl;

	// Read device info from the Python module
	parseInfo();

	// Configure uinput device
	uidev.id.bustype = BUS_VIRTUAL;

	if (devinfo.relaxiscount > 0) {
		ioctl(uifd, UI_SET_EVBIT, EV_REL);
		for (int i = 0; i < devinfo.relaxiscount; i++)
			ioctl(uifd, UI_SET_RELBIT, devinfo.relaxis[i]);
	}

	if (devinfo.absaxiscount > 0) {
		ioctl(uifd, UI_SET_EVBIT, EV_ABS);
		for (int i = 0; i < devinfo.absaxiscount; i++)
			ioctl(uifd, UI_SET_ABSBIT, devinfo.absaxis[i]);
	}

	if (devinfo.feedbackcount > 0) {
		ioctl(uifd, UI_SET_EVBIT, EV_FF);
		for (int i = 0; i < devinfo.feedbackcount; i++)
			ioctl(uifd, UI_SET_FFBIT, devinfo.feedback[i]);
	}

	if (devinfo.buttoncount > 0) {
		ioctl(uifd, UI_SET_EVBIT, EV_KEY);
		for (int i = 0; i < devinfo.buttoncount; i++)
			ioctl(uifd, UI_SET_KEYBIT, devinfo.buttons[i]);
	}

	//TODO: make it an option to specify the range on absolute axes (separately)
	for (int i = 0; i < ABS_MAX; i++) {
		uidev.absmin[i] = SHRT_MIN;
		uidev.absmax[i] = SHRT_MAX;
	}

	uidev.ff_effects_max = devinfo.maxeffects;

	errno = 0;

	int size = sizeof(uinput_user_dev);
	int w = write(uifd, &uidev, size);

	//TOOD:this part fails in ubuntu 12.04. find a solution
	//but it works in ubuntu 12.10
	if (w != sizeof(uinput_user_dev)) {
		cerr << "errno " << errno << " - " << strerror(errno) << endl;
		cerr << "(written " << w << " bytes)" << endl;
		throw(error("could not write device info"));
	}

	errno = 0;

	if (ioctl(uifd, UI_DEV_CREATE) != 0) {
		cerr << "errno " << errno << " - " << strerror(errno) << endl;
		throw(error("could not register device"));
	}

	cout << "\tDevice created." << endl;
	cout << "\tStarting control threads." << endl;
	cout << endl;

	//start the threads
	resume();
}

PyObject* device::moduleKillSelf(PyObject* self, PyObject* args) {
	pthread_mutex_lock(&vjoy::pymutex);
	Py_INCREF(Py_None);
	pthread_mutex_unlock(&vjoy::pymutex);

	cout << "\tkill command issued by module" << endl;

	dev->kill();

	return Py_None;
}

void device::kill() {
	pause();

	pthread_mutex_lock(&vjoy::pymutex);
	Py_DECREF(pymodule);
	pthread_mutex_unlock(&vjoy::pymutex);

	if (ioctl(uifd, UI_DEV_DESTROY) != 0) {
		//this really should never happen, since it would cause the process the stall indefinitely and make it 'immortal'
		//and possible prevent uinput from working until a reboot
		cerr << "errno " << errno << " - " << strerror(errno) << endl;
		throw(error("could not unregister device"));
	}
	close(uifd);
}

void device::pause() {
	setState(TO_STALL);
	pthread_join(inputThread, NULL);

	setState(STALLED);
}

void device::resume() {
	//this can only be called from the main thread
	currentStatus = LIVE;

	pthread_create(&inputThread, NULL, device::inputLoop, this);
}

