import vjoy, serial

ser = serial.Serial('/dev/serial/by-id/usb-FTDI_FT232R_USB_UART_A800evAX-if00-port0', 9600, timeout = 1)

#simulate an analog joystick
x = 0
y = 0

x_delta = 0
y_delta = 0

p = 200
f = 40

def balance(x):
	global p

	x = x / 2 + x / 4	#reduce
	if abs(x) <= p-1: x = 0 #stabilise

	return x

# This function returns essential joystick information
def getVJoyInfo():

	return {
		'name':	   'simple_joysitck', # The name of the virtual joystick
		'relaxis':	[], # List of relative axises to use
		'absaxis':	[vjoy.ABS_X, vjoy.ABS_Y], # List of absolute axises to use
		'feedback':   [], # List of force feedback types to support
		'maxeffects': 0, # Maximum number of concurrent feedback effects 
		'buttons':	[vjoy.BTN_X]  # List of buttons to use
	}


def doVJoyThink():
	global ser
	global x, y
	global x_delta, y_delta
	global p, f

	#everything in one byte
	ser.write(' ')
	data = ser.read(1)
	if len(data) == 0: return []
	
	data = ord(data)
	
	left =   (data >> 1) & 0x1
	right =  (data >> 3) & 0x1
	up =     (data >> 0) & 0x1
	down =   (data >> 2) & 0x1

	button = (data >> 4) & 0x1

	#assume left and right, up and down are mutually exclusive
	
	x = balance(x)
	y = balance(y)

	if left: 
		x -= p * f

	if right: 
		x += p * f
	
	if down: 
		y -= p * f

	if up: 
		y += p * f
	

	'''if left: 
		if x_delta > 0: x_delta = -p
		x_delta -= p

	if right: 
		if x_delta < 0: x_delta = p
		x_delta += p
	
	if down: 
		if y_delta > 0: y_delta = -p
		y_delta -= p

	if up: 
		if y_delta < 0: y_delta = p
		y_delta += p

	x += x_delta
	y += y_delta'''
	
	x = min([max([x, -32767]), 32767])
	y = min([max([y, -32767]), 32767])

	events = []
	events.append([vjoy.EV_ABS, vjoy.ABS_X, x])
	events.append([vjoy.EV_ABS, vjoy.ABS_Y, y])
	events.append([vjoy.EV_KEY, vjoy.BTN_X, button])
	
	return events

# Handle force feedback effect uploads.
def doVJoyUploadFeedback(effect):
	pass

# Erase a feedback effect
def doVJoyEraseFeedback(effectid):
	pass

