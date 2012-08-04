#ifndef MODULE_H
#define MODULE_H

#include <linux/input.h>
#include <linux/uinput.h>

#include <string>

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

#define DEFUALT_INPUT_RATE  60 // Loop input frequency in Hertz

class module {
private:
	void parseInfo();

	//threads
	static void* eventLoop(void* arg);
	static void* inputLoop(void* arg);

public:
	int uifd; // UInput File Descriptor
	uinput_user_dev uidev; // UInput Device Info

	PyObject* pymodule; // The Python script that operates this device
	vjoy_info devinfo; // The parsed device info

	pthread_t eventThread; // pthread structure for events
	pthread_t inputThread; // pthread for device input loop

	std::string name;
	int delay;

	module(const char* name) :
			name(name), delay(0) {
	}

	void setFrequency(int freq) {
		delay = 1000000 / freq;
	}

	void load();
};

#endif /* MODULE_H */
