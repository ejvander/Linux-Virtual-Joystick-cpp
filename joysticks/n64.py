# This a script interfer vjoy and an arduino connected to a N64 controller
# as describered here: http://www.instructables.com/id/Use-an-Arduino-with-an-N64-controller/?ALLSTEPS
# you'll need the sketch from the link (it's the N64_Arduino.zip file), 
# modify N64_Arduino.pde, comment line 349 to line 356
# and at line 348: "Serial.write((uint8_t*)&N64_status, sizeof(N64_status));"
# now compile and upload it into the arduino,
# Lauch vjoy whit this script and everything should work
# Also change the device path corresponding to your arduino to open serial connection

import vjoy # this module is hosted by the program
import math, serial, struct, time, ctypes

s = serial.Serial("/dev/ttyACM0", 115200, timeout = 1) # open arduino serial connection
s.bytesize = serial.EIGHTBITS
s.open()
time.sleep(3) # let time to the arduino to reboot and open serial connection

# This function returns essential joystick information
def getVJoyInfo():
	return {
		'name':       'n64_controller', 	# The name of the virtual joystick
		'relaxis':    [], 			# List of relative axises to use
		'absaxis':    [vjoy.ABS_X, vjoy.ABS_Y], # List of absolute axises to use
		'feedback':   [], 	# List of force feedback types to support
		'maxeffects': 0, 			# Maximum number of concurrent feedback effects 
		'buttons':    [vjoy.BTN_A, vjoy.BTN_B, 
			       vjoy.BTN_0, vjoy.BTN_1, vjoy.BTN_Z, 
			       vjoy.BTN_START, vjoy.BTN_4, vjoy.BTN_5, vjoy.BTN_6, vjoy.BTN_7, 
			       vjoy.BTN_3, vjoy.BTN_2, vjoy.BTN_LEFT, vjoy.BTN_RIGHT],  			# List of buttons to use
		'enable_ff': False			# Whether to enable for feedback
	}

# The "think" routine runs every few milliseconds.  Do NOT perform
# blocking operations within this function.  Doing so will prevent other
# important stuff from happening.

# The range of axes is signed short: [-32768, 32767]

def doVJoyThink():
    events = []
    data = [] 
    data = s.read(4) # we read the four byte containing the controller status

    intval = (struct.unpack('i',data[0:4]))[0] # we transform raw data into a number

    xval = (struct.unpack('<i', (struct.pack('<i', (intval & 0x00FF0000) >> 16))))[0]
    yval = (struct.unpack('<i', (struct.pack('<i', (intval & 0xFF000000) >> 24))))[0]
    if (yval > 127): # if yval > 127 then bit sign is set
       yval = yval - 256
    if (xval > 127):
       xval = xval - 256
    # we multiply by 512 to be in the [-32768, 32767] range
    events.append([vjoy.EV_ABS, vjoy.ABS_X, xval * 512]) # AxisX
    events.append([vjoy.EV_ABS, vjoy.ABS_Y, yval * 512]) # AxisY

    events.append([vjoy.EV_KEY, vjoy.BTN_LEFT, intval & 0x1])         # PadLeft
    events.append([vjoy.EV_KEY, vjoy.BTN_RIGHT, (intval & 0x2) >> 1]) # PadRight
    events.append([vjoy.EV_KEY, vjoy.BTN_2, (intval & 0x4) >> 2])     # PadUp
    events.append([vjoy.EV_KEY, vjoy.BTN_3, (intval & 0x8) >> 3])     # PadDown
    events.append([vjoy.EV_KEY, vjoy.BTN_START, (intval & 0x10) >> 4]) # Start
    events.append([vjoy.EV_KEY, vjoy.BTN_Z, (intval & 0x0020) >> 5])  # Z
    events.append([vjoy.EV_KEY, vjoy.BTN_B, (intval & 0x0040) >> 6])  # B
    events.append([vjoy.EV_KEY, vjoy.BTN_A, (intval & 0x0080) >> 7])  # A
    events.append([vjoy.EV_KEY, vjoy.BTN_6, (intval & 0x0100) >> 8])  # Cright
    events.append([vjoy.EV_KEY, vjoy.BTN_7, (intval & 0x0200) >> 9])  # Cleft
    events.append([vjoy.EV_KEY, vjoy.BTN_5, (intval & 0x0400) >> 10]) # Cdown
    events.append([vjoy.EV_KEY, vjoy.BTN_4, (intval & 0x0800) >> 11]) # Cup
    events.append([vjoy.EV_KEY, vjoy.BTN_1, (intval & 0x1000) >> 12]) # L
    events.append([vjoy.EV_KEY, vjoy.BTN_0, (intval & 0x2000) >> 13]) # R
    return events

# Handle force feedback effect uploads.
def doVJoyUploadFeedback(effect):
    pass

# Erase a feedback effect
def doVJoyEraseFeedback(effectid):
    pass

# Handle miscellaneous input events.  You probably won't need this.
def doVJoyEvent(evtype, evcode, evvalue):
    pass
