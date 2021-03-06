#ifndef SUBJOYSTICK
#define SUBJOYSTICK

#include "SDL/SDL.h"
#include <stdint.h>

class SubJoystick
{
    public:
    SubJoystick(); //Initializes
    int8_t * get_data(); //gets data to send
    void handle_input(SDL_Event myevent); //Handles joystick
    enum SubCommand {
		FORWARD = 0, // 1 for forward, -1 for backward, 0 for neither 
		LEFT, // 1 for left, -1 for right, 0 for neither
		PITCH, // 1 for up, -1 for down, 0 for neither
		ZOOM, // 1 for zoom in, -1 for zoom out, 0 for neither
		ASCEND, // 1 for ascend, -1 for descend, 0 for neither
		SHOOT, // 1 for "take a picture", 0 for don't
		LIGHTS, // 1 when lights should be on, 0 when lights should be off
		QUIT, //1 when we want to quit
		OPTION, //Hold Select and different things might happen!
        MODE, // 1 when we want to switch mode
        COMMAND_LENGTH  // A field to denote how many fields we have
	};
    
    private:
    int8_t commands[COMMAND_LENGTH];
    enum SubButtons {
		A_BUTTON = 0, // A Button (Descend)
		B_BUTTON, // B Button (Does Nothing)
		X_BUTTON, // X Button (Does Nothing)
		Y_BUTTON, // Y Button (Ascend)
		RL_BUTTON, // RL Button (Switches Mode)
		RB_BUTTON, // RB Button (Takes pictures)
		BACK_BUTTON, // Back Button (Does nothing)
		START_BUTTON // Start Button (Shuts down)
	};

};

#endif /* SUBJOYSTICK */
