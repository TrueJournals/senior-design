#include <iostream>
#include <unistd.h>
#include <libptp++.hpp>
#include <libusb-1.0/libusb.h>

// We need the sub joystick class so we can reuse our data struct
#include "../sd-surface/SubJoystick.hpp"
#include "Motor.hpp"
#include "../common/SignalHandler.hpp"
#include "submarine.hpp"

int main(int argv, char * argc[]) {
    int error;
    CHDKCamera cam;
    Motor subMotors[4]; // We need to control 4 motors
    SignalHandler signalHandler;
    LVData lv;
    uint8_t * lv_rgb;
    uint32_t size, width, height;
    uint16_t send_width, send_height;
    int8_t * joy_data;
    uint32_t joy_data_len;
    int cmd;
    int8_t sub_state[7]; // The current state of the submarine
    
    // Set up our signal handler(s)
    try {
        signalHandler.setupSignalHandlers();
    } catch(int e) {
        cout << "Fatal error: Unable to setup signal handler. Exception: " << e << endl;
        return 1;
    }
    
    // Keep trying to set up the camera until something tells us to stop
    while(setup_camera(&cam, &error) == false && signalHandler.gotExitSignal() == false) {
        cout << "Error setting up camera: " << error << " -- Trying again" << endl;
    }
    if(signalHandler.gotExitSignal() == true) {
        cout << "Fatal error: Failed to set up camera. Quitting. Last error: " << error << endl;
        return 2;
    }
    
    cout << "Camera is ready" << endl;
    cout << "CHDK Version: " << cam.get_chdk_version() << endl;
    
    // Initialize motors
    setup_motors(subMotors);
    cout << "Motors are ready" << endl;
    
    // TODO: Signal handler to allow us to quit loop when we receive SIGUSR1
    while(signalHandler.gotExitSignal() == false) {
        // TODO: Receive data
        
        // TODO: Motor control
        // NOTE: This is what I imagine receiving will be like. We might need
        //  to adjust this later.  Until Shawn updates SubJoystick, this will
        //  cause compilation to fail.
        // TODO: Put these in functions, call, ex., handle_forward, etc.
        // DONE: We can probably make this run faster by not doing the GPIO calls
        //  if we're already in the state we're trying to get to.  Make a compare_states function?
        
        if(compare_states(sub_state, joy_data) == false) {
            // Only run through these comparisons if our states have changed
            // Forward/backward
            if(sub_state[SubJoystick::LEFT] == 0) { // Left/right takes priority
                if(joy_data[SubJoystick::FORWARD] == 1) {
                    // If we want to go forward, and we're not trying to turn
                    subMotors[MOTOR_LEFT].spinForward();
                    subMotors[MOTOR_RIGHT].spinForward();
                    
                    sub_state[SubJoystick::FORWARD] = 1;
                } else if(joy_data[SubJoystick::FORWARD] == -1) {
                    // If we want to go backward, and we're not trying to turn
                    subMotors[MOTOR_LEFT].spinBackward();
                    subMotors[MOTOR_RIGHT].spinBackward();
                    
                    sub_state[SubJoystick::FORWARD] == -1;
                } else if(joy_data[SubJoystick::FORWARD] == 0) {
                    // If we want to go neither forward nor backward, and we're not trying to turn
                    subMotors[MOTOR_LEFT].stop();
                    subMotors[MOTOR_RIGHT].stop();
                    
                    sub_state[SubJoystick::FORWARD] = 0;
                }
            }
            
            // Left/right
            if(joy_data[SubJoystick::LEFT] == 1) {
                // If we want to turn left
                subMotors[MOTOR_LEFT].spinBackward();
                subMotors[MOTOR_RIGHT].spinForward();
                
                sub_state[SubJoystick::LEFT] = 1;
            } else if(joy_data[SubJoystick::LEFT] == -1) {
                // If we want to turn right
                subMotors[MOTOR_LEFT].spinForward();
                subMotors[MOTOR_RIGHT].spinBackward();
                
                sub_state[SubJoystick::LEFT] == -1;
            } else if(joy_data[SubJoystick::LEFT] == 0) {
                sub_state[SubJoystick::LEFT] = 0;
                
                if(sub_state[SubJoystick::FORWARD] == 0) {
                    // If we don't want to turn, and we aren't trying to move forward/backward
                    subMotors[MOTOR_LEFT].stop();
                    subMotors[MOTOR_RIGHT].stop();
                }
            }
            
            // Pitch up/down
            if(sub_state[SubJoystick::ASCEND] == 0) {
                // If we're not ascending or descending, we can pitch up/down
                if(joy_data[SubJoystick::PITCH] == -1) {
                    // If we want to pitch down and we're not ascending or descending
                    subMotors[MOTOR_TOP_FRONT].spinForward();
                    subMotors[MOTOR_TOP_REAR].spinBackward();
                    
                    sub_state[SubJoystick::PITCH] = -1;
                } else if(joy_data[SubJoystick::PITCH] == 1) {
                    // If we want to pitch up and we're not ascending or descending
                    subMotors[MOTOR_TOP_FRONT].spinBackward();
                    subMotors[MOTOR_TOP_REAR].spinForward();
                    
                    sub_state[SubJoystick::PITCH] = 1;
                } else if(joy_data[SubJoystick::PITCH] == 0) {
                    // If we want to stop pitching, and we're not ascending/descending
                    subMotors[MOTOR_TOP_FRONT].stop();
                    subMotors[MOTOR_TOP_REAR].stop();
                    
                    sub_state[SubJoystick::PITCH] = 0;
                }
            }
            
            // Ascend/descend
            if(joy_data[SubJoystick::ASCEND] == -1) {
                // If we want to descend
                subMotors[MOTOR_TOP_FRONT].spinForward();
                subMotors[MOTOR_TOP_REAR].spinForward();
                
                sub_state[SubJoystick::ASCEND] = -1;
            } else if(joy_data[SubJoystick::ASCEND] == 1) {
                // If we want to ascend
                subMotors[MOTOR_TOP_FRONT].spinBackward();
                subMotors[MOTOR_TOP_REAR].spinBackward();
                
                sub_state[SubJoystick::ASCEND] = 1;
            } else if(joy_data[SubJoystick::ASCEND] == 0) {
                sub_state[SubJoystick::ASCEND] = 0;
                
                if(sub_state[SubJoystick::PITCH] == 0) {
                    // If we don't want to ascend/descend, and we're not pitching
                    subMotors[MOTOR_TOP_FRONT].stop();
                    subMotors[MOTOR_TOP_REAR].stop();
                }
            }
            
            // Zoom in/out
            if(joy_data[SubJoystick::ZOOM] == -1) {
                // If we want to zoom out
                cam.execute_lua("click('zoom_in')", NULL);
                sub_state[SubJoystick::ZOOM] = -1;
            } else if(joy_data[SubJoystick::ZOOM] == 1) {
                // If we want to zoom in
                cam.execute_lua("click('zoom_out')", NULL);
                sub_state[SubJoystick::ZOOM] = 1;
            } else if(joy_data[SubJoystick::ZOOM] == 0) {
                sub_state[SubJoystick::ZOOM] = 0;
            }
            
            // Shoot
            if(joy_data[SubJoystick::SHOOT] == 1 && sub_state[SubJoystick::SHOOT] == 0) {
                // We don't want to shoot continuously.
                cam.execute_lua("shoot()", NULL);
                sub_state[SubJoystick::SHOOT] = 1;
            } else if(joy_data[SubJoystick::SHOOT] == 0 && sub_state[SubJoystick::SHOOT] == 1) {
                // Check current state so we're not doing this every loop iteration
                sub_state[SubJoystick::SHOOT] = 0;
            }
            
            // Lights
            if(joy_data[SubJoystick::LIGHTS] == 1 && sub_state[SubJoystick::LIGHTS] == 0) {
                // If the lights aren't already on
                // TODO: Turn the lights on. Which GPIO are we using for this?
            } else if(joy_data[SubJoystick::LIGHTS] == 0 && sub_state[SubJoystick::LIGHTS] == 1) {
                // Check current state so we're not doing this every loop iteration
                // TODO: Turn the lights off
            }
        }
        
        cam.get_live_view_data(&lv, true);
        lv_rgb = lv.get_rgb((int *)&size, (int *)&width, (int *)&height, true);
        
        send_width = 0xFFFF & width; // Just chop off the higher bytes
        send_height = 0xFFFF & height;
        // TODO: Send live view data
        //  Protocol: send size as four bytes, then width and height as two bytes
        //   then, send live view data
        
        free(lv_rgb);
    }
    
    cam.close();
    
    libusb_exit(NULL);
    
    return 0;
}

bool setup_camera(CHDKCamera * cam, int * error) {
    libusb_device * dev;
    
    *error = libusb_init(NULL);
    if(*error < 0) {
        return false;
    }
    
    *error = 0;
    
    dev = CHDKCamera::find_first_camera();
    
    if(dev == NULL) {
        *error = 1;
        return false;
    }
    
    try {
        cam->open(dev);
    } catch(int e) {
        *error = e;
        return false;
    }
    
    cam->execute_lua("switch_mode_usb(1)", NULL); // TODO: block instead of sleep?
    sleep(1);
    cam->execute_lua("set_prop(121, 1)", NULL); // Set flash to manual adjustment
    usleep(500 * 10^3);   // Sleep for half a second -- TODO: Block instead?
    cam->execute_lua("set_prop(143, 2)", NULL); // Set flash mode to off
    usleep(500 * 10^3);
    
    return true;
}

void setup_motors(Motor * subMotors) {
    int i;
    
    for(i=0;i<4;i++) {
        subMotors[i].setup(MOTOR_PINS[i]);
    }
}

bool compare_states(int * sub_state, int * joy_data) {
    // Returns true if the states are the same, false otherwise
    for(int i=0; i < 7; i++) {
        if(*(sub_state+i) != *(joy_data+i)) return false;
    }
    
    return true;
}
