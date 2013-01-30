#!/usr/bin/python

import time
import threading
import random
from ptp2 import *
import socket
import RPi.GPIO as GPIO

# Small class for dealing with motors
class Motor:
    # You can use this in spin(direction) if you want
    OFF = 0
    FORWARD = 1
    REVERSE = 2
    BACKWARD = 2
    
    # Pins should be given in a tuple
    def __init__(self, pins, gpio):
        self.pins = pins
        self.gpio = gpio
        
        # Automatically setup
        self.setup()
        
    def setup(self):
        for pin in self.pins:
            self.gpio.setup(pin, self.gpio.out)
    
    def spinForward(self):
        self.gpio.output(self.pins[0], False) # Always turn one off first
        self.gpio.output(self.pins[1], True)  # Doing so ensure we never send high to both H-bridge inputs
        
    def spinBackward(self):
        self.gpio.output(self.pins[1], False) # Always turn one off first
        self.gpio.output(self.pins[0], True)
        
    def spinReverse(self):
        self.spinBackward() # Just an alias
    
    def stop(self):
        self.gpio.output(self.pins[0], False)
        self.gpio.output(self.pins[1], False)
        
    def spin(self, direction):
        # direction should be 0-2
        if direction == self.OFF:
            self.stop()
        elif direction == self.FORWARD:
            self.spinForward()
        elif direction == self.REVERSE:
            self.spinBackward()
        else:
            pass # TODO: Throw an exception

# Setup socket
host = ''
port = 50000
backlog = 5
size = 1024*1024*5
#current direction
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.bind((host,port))
s.listen(backlog)
print 'server running on port', port, 'at', socket.gethostbyname(socket.gethostname())

# Set up GPIO
motors = [Motor((18, 23), GPIO), Motor((24, 25), GPIO), Motor((8, 7), GPIO), Motor((17, 27), GPIO)]
for motor in motors:    # Ensure all motors are stopped to begin with
    motor.stop()
print "GPIO is ready!"
#GPIO.setmode(GPIO.BCM)
#for pin in (18, 23, 24, 25, 8, 7, 17, 27):
#    GPIO.setup(pin, GPIO.OUT)

# Set up camera
cam = CHDKCamera(util.list_ptp_cameras()[0])
cam.execute_lua("switch_mode_usb(1)")
time.sleep(1)
cam.execute_lua("set_prop(121, 1)") # Set flash to manual adjustment
time.sleep(0.5)
cam.execute_lua("set_prop(143, 2)") # Property 16 is flash mode, 2 is off
time.sleep(0.5)
# Camera set up
print "Camera is ready!"

state = {
        'shoot': False, 'zoom_in': False, 'zoom_out': False,    # Camera functions
        'forward': False, 'backward': False,                    # Motors 1 and 2
        'left': False, 'right': False,                          # Motors 1 and 2
        'descend': False, 'ascend': False,                      # Motors 3 and 4
        'pitch_up': False, 'pitch_down': False                  # Motors 3 and 4
        }

while True:
    data = []
    client, address = s.accept()
    data = client.recv(size)
    # TODO: Build a protocol
    #  But, we're actually only sending one type of data in either direction...
    #  Determine maximum size of received string, and change "size" above
    
    #while True:
    #    r = client.recv(size)
    #    if not r:
    #        break
    #    else:
    #        data.append(r)
    if data:
        full_r = "".join(data)
        # print 'string received size', len(full_r)
        # forward/back||left/right||pitch up/down||zoom out||zoom in||descend||ascend||take picture||E
        print full_r
        commands = full_r.split('||')
        for i in range(len(commands)):
            cmd = float(commands[i])
            # TODO: Wow, this is difficult to read...
            if i == 7:
                if cmd == 1 and state['shoot'] == False:
                    cam.execute_lua('shoot()')
                    state['shoot'] = True
                elif cmd == 0 and state['shoot'] == True:
                    state['shoot'] = False
            elif i == 3:
                if cmd == 1 and state['zoom_out'] == False and state['zoom_in'] == False: #Can't zoom in and out at the same time
                    cam.execute_lua("click('zoom_out')")
                    state['zoom_out'] = True
                elif cmd == 0 and state['zoom_out'] == True:
                    state['zoom_out'] = False
            elif i == 4:
                if cmd == 1 and state['zoom_in'] == False and state['zoom_out'] == False:
                    cam.execute_lua("click('zoom_in')")
                    state['zoom_in'] = True
                elif cmd == 0 and state['zoom_in'] == True:
                    state['zoom_in'] = False
            elif i == 0:
                # Forward/backward
                if cmd == 1 and state['backward'] == False and (state['left'] == False and state['right'] == False):
                    # Motor 1 backward
                    motors[0].spinBackward()
                    # Motor 2 backward
                    motors[1].spinBackward()
                    
                    # Set state
                    state['backward'] = True
                    state['forward'] = False
                elif cmd == -1 and state['forward'] == False and (state['left'] == False and state['right'] == False):
                    # Motor 1 forward
                    motors[0].spinForward()
                    # Motor 2 forward
                    motors[1].spinForward()
                    
                    # Set state
                    state['forward'] = True
                    state['backward'] = False
                elif cmd == 0 and state['left'] == False and state['right'] == False:
                    # Motor 1 off
                    motors[0].stop()
                    # Motor 2 off
                    motors[1].stop()
                    
                    # Set state
                    state['forward'] = False
                    state['backward'] = False
            elif i == 1:
                # Left/right
                if cmd == 1 and state['left'] == False:
                    # Motor 1 forward
                    motors[0].spinForward()
                    # Motor 2 backward
                    motors[1].spinBackward()
                    
                    state['left'] = True
                    state['right'] = False
                elif cmd == -1 and state['right'] == False:
                    # Motor 1 backward
                    motors[0].spinBackward()
                    # Motor 2 forward
                    motors[1].spinForward()
                    
                    state['right'] = True
                    state['left'] = False
                elif cmd == 0 and state['forward'] == False and state['backward'] == False:
                    # Motor 1 off
                    motors[0].stop()
                    # Motor 2 off
                    motors[1].stop()
                    
                    # Set state
                    state['left'] = False
                    state['right'] = False
            # TODO: examine ascend/descend / tilt conflict cases
            elif i == 5:
                # Descend
                if cmd == 1 and state['descend'] == False and state['ascend'] == False:
                    # Motor 3 backward
                    motors[2].spinBackward()
                    # Motor 4 backward
                    motors[3].spinBackward()
                    
                    # Set state
                    state['descend'] = True
                elif cmd == 0 and state['ascend'] == False and (state['pitch_up'] == False and state['pitch_down'] == False):
                    # Motor 3 off
                    motors[2].stop()
                    # Motor 4 off
                    motors[3].stop()
                    
                    # Set state
                    state['descend'] = False
            elif i == 6:
                # Ascend
                if cmd == 1 and state['ascend'] == False and state['descend'] == False:
                    # Motor 3 forward
                    motors[2].spinForward()
                    # Motor 4 forward
                    motors[3].spinForward()
                    
                    # Set state
                    state['ascend'] = True
                elif cmd == 0 and state['descend'] == False and (state['pitch_up'] == False and state['pitch_down'] == False):
                    # Motor 3 off
                    motors[2].stop()
                    # Motor 4 off
                    motors[3].stop()
                    
                    # Set state
                    state['ascend'] = False
            elif i == 2:
                # Pitch up/down
                if cmd == 1 and state['pitch_down'] == False and (state['ascend'] == False and state['descend'] == False):
                    # Motor 3 forward
                    motors[2].spinForward()
                    # Motor 4 backward
                    motors[3].spinBackward()
                    
                    # Set state
                    state['pitch_down'] = True
                    state['pitch_up'] = False
                elif cmd == -1 and state['pitch_up'] == False and (state['ascend'] == False and state['descend'] == False):
                    # Motor 3 backward
                    motors[2].spinBackward()
                    # Motor 4 forward
                    motors[3].spinForward
                    
                    # Set state
                    state['pitch_up'] = True
                    state['pitch_down'] = False
                elif cmd == 0 and state['ascend'] == False and state['descend'] == False:
                    # Motor 3 off
                    motors[2].stop()
                    # Motor 4 off
                    motors[3].stop()
                    
                    # Set state
                    state['pitch_up'] = False
                    state['pitch_down'] = False
                    

        #img = chdkimage.convertColorspace(cam.get_live_view_data()[1].vp_data, 0, 360, 480)
        #print len(img)
        #client.sendall(img)
        #client.send(img)
    print state                
    client.close()
    
cam.close()
