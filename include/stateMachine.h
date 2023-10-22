#include <stdio.h>
#include "global_macros.h"

/**
 * @brief Enumeration representing the possible states of the state machine.
 */
typedef enum {
    START,        ///< Initial state.
    FLAG_RCV,     ///< Received starting flag.
    A_RCV,        ///< Received address flag.
    C_RCV,        ///< Received control flag.
    BCC_OK,       ///< Received BCC and it is valid.
    STOP          ///< End state.
} State;

/**
 * @brief Structure representing a state machine.
 */
typedef struct {
    State currentState;     ///< Current state of the state machine.
    unsigned char controlByte; ///< Control byte associated with the state.
    int addressNumber;      ///< Address associated with the state.
    int frameSize;          ///< Size of the frame.
} StateMachine;

/**
 * @brief Enumeration representing the modes of the state machine.
 */
typedef enum {
    Communication,  ///< Communication mode.
    Data            ///< Data mode.
} Mode;

/**
 * @brief Function to change the state of the state machine.
 *
 * @param machine Pointer to the StateMachine structure.
 * @param state The new state to set.
 */
void changeState(StateMachine *machine, State state);

/**
 * @brief Function to handle the state transitions and processing of incoming data.
 *
 * @param machine Pointer to the StateMachine structure.
 * @param byteToProcess The byte to be processed by the state machine.
 * @param frame Pointer to the frame data.
 * @param mode The mode of the state machine (Communication or Data).
 */
void StateHandler(StateMachine *machine, unsigned char byteToProcess, unsigned char* frame, Mode mode);

