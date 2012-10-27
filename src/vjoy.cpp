#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <cwctype>

#include <cassert>
#include <errno.h>

#include <iostream>
#include <string>
#include <sstream>

#include <vector>
#include <list>

#include <fcntl.h>
#include <unistd.h>

#include "python_include.h"
#include "vjoy.h"
#include "vjoy_python.h"

using namespace std;

//global
pthread_mutex_t vjoy::pymutex;

//this thread is a 'server' it processes events coming from applications, such as force feedback
//this thread processes input coming from the python script, possibly relayed from external hardware
void* device::inputLoop(void* arg) {
	device* dev = (device*) arg;

	//feedback from uinput
	input_event evt;
	uinput_ff_upload ureq;
	uinput_ff_erase ereq;

	PyObject* res;
	PyObject* pyeffect;

	//events from hardware - send to uinput
	PyObject* pyevents;

	while (true) {
		if (dev->getState() == TO_STALL) pthread_exit(NULL);

		if (dev->enableFF) {
			//non-blocking
			int s = read(dev->uifd, &evt, sizeof(input_event));

			//error
			if (s == -1) {
				if (errno != EAGAIN) {
					cerr << "could not read device file; errno is " << errno << " - " << strerror(errno) << endl;
					ioctl(dev->uifd, UI_DEV_DESTROY);
					pthread_exit(NULL);
				}
			}

			else {
				cout << "ff " << s << endl;
				//process events
				switch (evt.type) {

				case EV_UINPUT:
					switch (evt.code) {

					case UI_FF_UPLOAD:
						memset(&ureq, 0, sizeof(struct uinput_ff_upload));
						ureq.request_id = evt.value;
						ioctl(dev->uifd, UI_BEGIN_FF_UPLOAD, &ureq);

						pthread_mutex_lock(&vjoy::pymutex);
						pyeffect = convert_ff_effect(&ureq.effect);
						res = PyObject_CallMethod(dev->pymodule, const_cast<char*>("doVJoyUploadFeedback"), const_cast<char*>("O"), pyeffect);
						Py_XDECREF(res);

						if (PyErr_Occurred()) {
							PyErr_Print();
						}
						pthread_mutex_unlock(&vjoy::pymutex);

						ioctl(dev->uifd, UI_END_FF_UPLOAD, &ureq);
						break;

					case UI_FF_ERASE:
						memset(&ereq, 0, sizeof(struct uinput_ff_erase));
						ereq.request_id = evt.value;
						ioctl(dev->uifd, UI_BEGIN_FF_ERASE, &ereq);

						pthread_mutex_lock(&vjoy::pymutex);
						res = PyObject_CallMethod(dev->pymodule, const_cast<char*>("doVJoyEraseFeedback"), const_cast<char*>("i"), ereq.effect_id);
						Py_XDECREF(res);

						if (PyErr_Occurred()) {
							PyErr_Print();
						}
						pthread_mutex_unlock(&vjoy::pymutex);

						ioctl(dev->uifd, UI_END_FF_ERASE, &ereq);
						break;

					default:
						break;
					}

					break;

				default:
					//the event can't be handled, so sent it to the python script
					pthread_mutex_lock(&vjoy::pymutex);

					res = PyObject_CallMethod(dev->pymodule, const_cast<char*>("doVJoyEvent"), const_cast<char*>("iii"), evt.type, evt.code, evt.value);
					Py_XDECREF(res);

					if (PyErr_Occurred()) {
						PyErr_Print();
					}

					pthread_mutex_unlock(&vjoy::pymutex);
					break;
				}
			}
		}

		//events
		pthread_mutex_lock(&vjoy::pymutex);
		pyevents = PyObject_CallMethod(dev->pymodule, const_cast<char*>("doVJoyThink"), NULL);

		if (PyErr_Occurred()) {
			PyErr_Print();
			cout << "python error in thread execution" << endl;
			ioctl(dev->uifd, UI_DEV_DESTROY);
			pthread_exit(NULL);
		}

		if (pyevents) {
			int eventcount = PySequence_Size(pyevents);

			if (eventcount > 0) {
				memset(&evt, 0, sizeof(struct input_event));
				gettimeofday(&evt.time, NULL);

				// TODO: This all needs more error checking
				for (int i = 0; i < eventcount; i++) {
					PyObject* pyevent = PySequence_GetItem(pyevents, i);

					if (pyevent) {
						if (PySequence_Size(pyevent) != 3) {
							cerr << "Event lists must have exactly three items in the form (type, code, value)" << endl;
							continue;
						}

						PyObject* pytype = PySequence_GetItem(pyevent, 0);
						if (pytype) {
							evt.type = PyLong_AsLong(pytype);
							Py_DECREF(pytype);
						}

						PyObject* pycode = PySequence_GetItem(pyevent, 1);
						if (pycode) {
							evt.code = PyLong_AsLong(pycode);
							Py_DECREF(pycode);
						}

						PyObject* pyvalue = PySequence_GetItem(pyevent, 2);
						if (pyvalue) {
							evt.value = PyLong_AsLong(pyvalue);
							Py_DECREF(pyvalue);
						}

						Py_DECREF(pyevent);
					}

					write(dev->uifd, &evt, sizeof(struct input_event));
				}

				evt.type = EV_SYN;
				evt.code = SYN_REPORT;
				evt.value = 0;
				write(dev->uifd, &evt, sizeof(struct input_event));
			}

			Py_DECREF(pyevents);
		}

		pthread_mutex_unlock(&vjoy::pymutex);
		usleep(dev->delay);
	}
}

PyObject* convert_ff_envelope(struct ff_envelope* envelope) {
	PyObject* pyenvelope = PyDict_New();
	PyDict_SetItemString(pyenvelope, "attack_length", PyLong_FromLong(envelope->attack_length));
	PyDict_SetItemString(pyenvelope, "attack_level", PyLong_FromLong(envelope->attack_level));
	PyDict_SetItemString(pyenvelope, "fade_length", PyLong_FromLong(envelope->fade_length));
	PyDict_SetItemString(pyenvelope, "fade_level", PyLong_FromLong(envelope->fade_level));

	return pyenvelope;
}

PyObject* convert_ff_effect(struct ff_effect* effect) {
	PyObject* pyeffect = PyDict_New();
	PyDict_SetItemString(pyeffect, "type", PyLong_FromLong(effect->type));
	PyDict_SetItemString(pyeffect, "id", PyLong_FromLong(effect->id));
	PyDict_SetItemString(pyeffect, "direction", PyLong_FromLong(effect->direction));

	PyObject* pytrigger = PyDict_New();
	PyDict_SetItemString(pytrigger, "button", PyLong_FromLong(effect->trigger.button));
	PyDict_SetItemString(pytrigger, "interval", PyLong_FromLong(effect->trigger.interval));
	PyDict_SetItemString(pyeffect, "trigger", pytrigger);

	PyObject* pyreplay = PyDict_New();
	PyDict_SetItemString(pyreplay, "length", PyLong_FromLong(effect->replay.length));
	PyDict_SetItemString(pyreplay, "delay", PyLong_FromLong(effect->replay.delay));
	PyDict_SetItemString(pyeffect, "replay", pyreplay);

	PyObject* pysubeffect[2] =
		{ NULL, NULL };

	switch (effect->type) {
	case FF_CONSTANT:
		pysubeffect[0] = PyDict_New();
		PyDict_SetItemString(pysubeffect[0], "level", PyLong_FromLong(effect->u.constant.level));
		PyDict_SetItemString(pysubeffect[0], "envelope", convert_ff_envelope(&effect->u.constant.envelope));
		PyDict_SetItemString(pyeffect, "constant", pysubeffect[0]);
		break;

	case FF_PERIODIC:
		pysubeffect[0] = PyDict_New();
		PyDict_SetItemString(pysubeffect[0], "waveform", PyLong_FromLong(effect->u.periodic.waveform));
		PyDict_SetItemString(pysubeffect[0], "period", PyLong_FromLong(effect->u.periodic.period));
		PyDict_SetItemString(pysubeffect[0], "magnitude", PyLong_FromLong(effect->u.periodic.magnitude));
		PyDict_SetItemString(pysubeffect[0], "offset", PyLong_FromLong(effect->u.periodic.offset));
		PyDict_SetItemString(pysubeffect[0], "phase", PyLong_FromLong(effect->u.periodic.phase));
		PyDict_SetItemString(pysubeffect[0], "envelope", convert_ff_envelope(&effect->u.periodic.envelope));
		PyDict_SetItemString(pyeffect, "periodic", pysubeffect[0]);
		break;

	case FF_RAMP:
		pysubeffect[0] = PyDict_New();
		PyDict_SetItemString(pysubeffect[0], "start_level", PyLong_FromLong(effect->u.ramp.start_level));
		PyDict_SetItemString(pysubeffect[0], "end_level", PyLong_FromLong(effect->u.ramp.end_level));
		PyDict_SetItemString(pysubeffect[0], "envelope", convert_ff_envelope(&effect->u.ramp.envelope));
		PyDict_SetItemString(pyeffect, "ramp", pysubeffect[0]);
		break;

	case FF_SPRING:
	case FF_FRICTION:
		pysubeffect[0] = PyDict_New();
		pysubeffect[1] = PyDict_New();
		PyDict_SetItemString(pysubeffect[0], "right_saturation", PyLong_FromLong(effect->u.condition[0].right_saturation));
		PyDict_SetItemString(pysubeffect[0], "left_saturation", PyLong_FromLong(effect->u.condition[0].left_saturation));
		PyDict_SetItemString(pysubeffect[0], "right_coeff", PyLong_FromLong(effect->u.condition[0].right_coeff));
		PyDict_SetItemString(pysubeffect[0], "left_coeff", PyLong_FromLong(effect->u.condition[0].left_coeff));
		PyDict_SetItemString(pysubeffect[0], "deadband", PyLong_FromLong(effect->u.condition[0].deadband));
		PyDict_SetItemString(pysubeffect[0], "center", PyLong_FromLong(effect->u.condition[0].center));
		PyDict_SetItemString(pysubeffect[1], "right_saturation", PyLong_FromLong(effect->u.condition[1].right_saturation));
		PyDict_SetItemString(pysubeffect[1], "left_saturation", PyLong_FromLong(effect->u.condition[1].left_saturation));
		PyDict_SetItemString(pysubeffect[1], "right_coeff", PyLong_FromLong(effect->u.condition[1].right_coeff));
		PyDict_SetItemString(pysubeffect[1], "left_coeff", PyLong_FromLong(effect->u.condition[1].left_coeff));
		PyDict_SetItemString(pysubeffect[1], "deadband", PyLong_FromLong(effect->u.condition[1].deadband));
		PyDict_SetItemString(pysubeffect[1], "center", PyLong_FromLong(effect->u.condition[1].center));
		PyDict_SetItemString(pyeffect, "condition", PyTuple_Pack(2, pysubeffect[0], pysubeffect[1]));
		break;

	case FF_RUMBLE:
		pysubeffect[0] = PyDict_New();
		PyDict_SetItemString(pysubeffect[0], "strong_magnitude", PyLong_FromLong(effect->u.rumble.strong_magnitude));
		PyDict_SetItemString(pysubeffect[0], "weak_magnitude", PyLong_FromLong(effect->u.rumble.weak_magnitude));
		PyDict_SetItemString(pyeffect, "rumble", pysubeffect[0]);
		break;

	default:
		break;
	}

	return pyeffect;
}

void vjoy::init() {
	cout << "Initializing..." << endl;

#ifdef PYTHON3
	PyImport_AppendInittab("vjoy", PyInit_vjoy);
#else
	PyImport_AppendInittab("vjoy", initvjoy);
#endif

	Py_Initialize();
	if (PyErr_Occurred()) {
		PyErr_Print();
		throw(error("could not initialize python"));
	}

	if (pthread_mutex_init(&vjoy::pymutex, NULL)) throw(error("mutex initialization failed"));

	cout << endl;
	cout << "looking for the modules in: " << endl;

#ifdef PYTHON3
	wstringstream path;

	char cwd[FILENAME_MAX];
	getcwd(cwd, FILENAME_MAX);

	path << Py_GetPath() << ":" << getenv("HOME") << "/.config/vjoy/modules:" << cwd;

	cout << endl;

	wstring _p = path.str();
	size_t f = 0;
	size_t last = 0;

	while (f != wstring::npos) {
		last = f;
		f = _p.find(L":", f + 1);
		if (last == 0) wcout << "\t" << _p.substr(last, f - last - 1) << endl;
		else wcout << "\t" << _p.substr(last + 1, f - last - 1) << endl;
	}

	PySys_SetPath(path.str().c_str());
#else

	char cwd[FILENAME_MAX];
	getcwd(cwd, FILENAME_MAX);

	PyObject* sysModule = PyImport_ImportModule("sys");
	PyObject* pyPath = PyObject_GetAttrString(sysModule, "path");

	PyList_Append(pyPath, PyString_FromString("."));
	if (getenv("HOME")) PyList_Append(pyPath, PyString_FromString(getenv("HOME")));
	PyList_Append(pyPath, PyString_FromString(cwd));

#endif

	cout << endl;
	cout << "Finished initialization." << endl << endl;
}
