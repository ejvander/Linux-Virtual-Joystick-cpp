try:
  import vjoy # this module is hosted by the program
  import time
  import math
except ImportError:
  print "what?"

# This function returns essential joystick information
def getVJoyInfo():
	return {
		'name':       'gamepad', 	# The name of the virtual joystick
		'relaxis':    [], 			# List of relative axises to use
		'absaxis':    [vjoy.ABS_X, vjoy.ABS_Y], # List of absolute axises to use
		'feedback':   [vjoy.FF_RUMBLE], 	# List of force feedback types to support
		'maxeffects': 0, 			# Maximum number of concurrent feedback effects 
		'buttons':    [vjoy.BTN_A, vjoy.BTN_B,
                                vjoy.BTN_0, vjoy.BTN_1, vjoy.BTN_Z, vjoy.BTN_X, vjoy.BTN_Y, vjoy.BTN_SELECT,
                                vjoy.BTN_START, vjoy.BTN_4, vjoy.BTN_5, vjoy.BTN_6, vjoy.BTN_7,
                                vjoy.BTN_3, vjoy.BTN_2, vjoy.BTN_LEFT, vjoy.BTN_RIGHT],  			# List of buttons to use
		'enable_ff': False			# Whether to enable for feedback
	}

# The "think" routine runs every few milliseconds.  Do NOT perform
# blocking operations within this function.  Doing so will prevent other
# important stuff from happening.

# The range of axes is signed short: [-32768, 32767]

theta = 0.0
way = True
timez = 0
jinput = open("/tmp/in", "r+")

event = None

def doVJoyThink():
    global theta, way, timez, event

    events = []

    newTime = int(time.time())
    if newTime - timez > 1:
        timez = newTime
        readStr = jinput.read()
        readStr = readStr.strip() 
   
        if readStr != "":
            print "Got following: \"" + readStr + "\""
            if readStr == "a" or readStr == "A":
                print "was a"
                event = [vjoy.EV_KEY, vjoy.BTN_A, 1]
            elif readStr == "b":
                print "was b"
                event = [vjoy.EV_KEY, vjoy.BTN_B, 1]
            elif readStr == "x":
                print "was x"
                event = [vjoy.EV_KEY, vjoy.BTN_Z, 1]
            elif readStr == "y":
                print "was y"
                event = [vjoy.EV_KEY, vjoy.BTN_Y, 1]
            elif readStr == "s":
                print "was s"
                event = [vjoy.EV_KEY, vjoy.BTN_START, 1]
            elif readStr == "e":
                print "was e"
                event = [vjoy.EV_KEY, vjoy.BTN_SELECT, 1]
            elif readStr == "0":
                print "was 0"
                event = [vjoy.EV_KEY, vjoy.BTN_0, 1]
            elif readStr == "1":
                print "was 1"
                event = [vjoy.EV_KEY, vjoy.BTN_1, 1]
            elif readStr == "2":
                print "was 2"
                event = [vjoy.EV_KEY, vjoy.BTN_2, 1]
            elif readStr == "u":
                print "was u"
                event = [vjoy.EV_ABS, vjoy.ABS_Y, 32767]
            elif readStr == "d":
                print "was d"
                event = [vjoy.EV_ABS, vjoy.ABS_Y, -32767]
            elif readStr == "l":
                print "was l"
                event = [vjoy.EV_ABS, vjoy.ABS_X, -32767]
            elif readStr == "r":
                print "was r"
                event = [vjoy.EV_ABS, vjoy.ABS_X, 32767]
            else:
                print "unknown"
            jinput.seek(0)
            jinput.truncate()
        else:
            if event is not None:
                event[2] = 0
                events.append(event)
            event = None

    if event is not None:
        events.append(event)
    
    return events

# Handle force feedback effect uploads.
def doVJoyUploadFeedback(effect):
    print ("Feedback Upload:")
    print ("\tID:", effect['id'])
    print ("\tStrong Magnitude:", effect['rumble']['strong_magnitude'])
    print ("\tWeak Magnitude:", effect['rumble']['weak_magnitude'])

# Erase a feedback effect
def doVJoyEraseFeedback(effectid):
    print ("Feedback Erase:")
    print ("\tID:", effectid)

# Handle miscellaneous input events.  You probably won't need this.
def doVJoyEvent(evtype, evcode, evvalue):
    pass
