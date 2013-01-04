import vjoy # this module is hosted by the program
import math

# This function returns essential joystick information
def getVJoyInfo():
	return {
		'name':       'example_device', 	# The name of the virtual joystick
		'relaxis':    [], 			# List of relative axises to use
		'absaxis':    [vjoy.ABS_X, vjoy.ABS_Y], # List of absolute axises to use
		'feedback':   [vjoy.FF_RUMBLE], 	# List of force feedback types to support
		'maxeffects': 0, 			# Maximum number of concurrent feedback effects 
		'buttons':    [],  			# List of buttons to use
		'enable_ff': False			# Whether to enable for feedback
	}

# The "think" routine runs every few milliseconds.  Do NOT perform
# blocking operations within this function.  Doing so will prevent other
# important stuff from happening.

# The range of axes is signed short: [-32768, 32767]

theta = 0.0
way = True

def doVJoyThink():
    global theta, way

    if (math.fabs(theta) >= math.pi): way = not way;
    if (way): theta += 0.05 
    else: theta -= 0.05

    events = []
    x      = int(math.cos(theta) * 32500)
    y      = int(math.sin(theta) * 32500)
    events.append([vjoy.EV_ABS, vjoy.ABS_X, x])
    events.append([vjoy.EV_ABS, vjoy.ABS_Y, y])
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
