#include <stdio.h>
#include "../src/macros.h"


// Define states
typedef enum {
    START, 
    FLAG_RCV, //RECEIVED STARTING FLAG
    A_RCV, // RECEIVED ADDRESS FLAG
    C_RCV, // RECEIVED CONTROL FLAG
    BCC_OK, // RECEIVED BCC AND IT IS OK
    STOP
} State;

// Define event
typedef enum {
    EVENT_START,
    EVENT_END
} Event;

// Define the state machine structure
typedef struct {
    State currentState;
} StateMachine;

// Define the two modes of state machine that it will have
typedef enum {
    Communication,
    Data
} Mode;

void changeState(StateMachine *machine, State state);
void StateHandler(StateMachine *machine, unsigned char byteToProcess, unsigned char* frame, Mode mode);