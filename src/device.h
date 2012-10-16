#ifndef DEVICE_H
#define DEVICE_H

#include <linux/input.h>
#include <linux/uinput.h>

#include <string>
#include <ostream>
#include <iostream>

#include "python_include.h"

typedef struct _vjoy_info {
	char name[UINPUT_MAX_NAME_SIZE];

	int relaxis[REL_CNT];
	int relaxiscount;

	int absaxis[ABS_CNT];
	int absaxiscount;

	int feedback[FF_CNT];
	int feedbackcount;
	int maxeffects;

	int buttons[KEY_CNT];
	int buttoncount;
} vjoy_info;

#define DEFUALT_INPUT_RATE  60 // default loop input frequency in Hertz
class device {
public:
	static device* dev;

	enum dev_state {
		LIVE = 0, TO_STALL, STALLED
	};

	char* devPath;
	int uifd; // UInput File Descriptor
	uinput_user_dev uidev; // UInput Device Info

	PyObject* pymodule; // The Python script that operates this device
	vjoy_info devinfo; // The parsed device info

	pthread_t eventThread; // pthread structure for events
	pthread_t inputThread; // pthread for device input loop

	std::string name; //sadly this is a duplicate
	int delay; //milliseconds

	//this is called from inputLoop
	dev_state getState() {
		pthread_mutex_lock(&devMutex);
		dev_state ret = currentStatus;
		pthread_mutex_unlock(&devMutex);
		return ret;
	}

	void setState(const dev_state& s) {
		pthread_mutex_lock(&devMutex);
		currentStatus = s;
		pthread_mutex_unlock(&devMutex);
	}

	device(std::string devName, int freq) :
			name(devName) {
		if (freq == 0) freq = DEFUALT_INPUT_RATE;
		delay = 1000000 / freq;

		currentStatus = LIVE;

		pause_callback = NULL;
		resume_callback = NULL;
		kill_callback = NULL;
	}

	~device() {

	}

	//this can be called from one of the threads
	std::ostream& err() {
		pthread_mutex_lock(&devMutex);
		return std::cerr << name << ": ";
		pthread_mutex_unlock(&devMutex);
	}

	void load();

	//this can also be called from the event thread
	void kill();

	//these are to be called only from main thread
	void pause();
	void resume();

private:
	//these are module functions called when appropriate, if they exist
	PyObject* pause_callback;
	PyObject* resume_callback;
	PyObject* kill_callback;

	//threads
	static void* eventLoop(void* arg);
	static void* inputLoop(void* arg);

	void killEventLoop();
	void killInputLoop();

	static pthread_mutex_t devMutex; //this is for indirectly communicating with the main loop


	static PyObject* moduleKillSelf(PyObject* self, PyObject* args); //this is a python callback

	void parseInfo();

	pthread_cond_t stallCond; //this waited for in the case of stall

	dev_state currentStatus;
};

#endif /* DEVICE_H */
