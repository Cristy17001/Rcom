#include "stateMachine.h"

void changeState(StateMachine *machine, State state) {
    machine->currentState = state;
}

// Function to handle changes in states
void StateHandler(StateMachine *machine, unsigned char byteToProcess, unsigned char* frame, Mode mode) {
    if (mode == Communication) {
        switch (machine->currentState) {
        case START:
            if (byteToProcess == FLAG) {
                changeState(machine, FLAG_RCV);
                frame[0] = byteToProcess;
            }
            break;
        case FLAG_RCV:
            if (byteToProcess == FLAG) break;
            if (byteToProcess == ADDRESS_1) {
                changeState(machine, A_RCV);
                frame[1] = byteToProcess;
            }
            else changeState(machine, START);
            break;
        case A_RCV:
            if (byteToProcess == FLAG) {changeState(machine, FLAG_RCV); break;}
            if (byteToProcess == machine->controlByte) {
                changeState(machine, C_RCV);
                frame[2] = byteToProcess;
            }
            else changeState(machine, START);
            break;
        case C_RCV:
            if (byteToProcess == FLAG) {changeState(machine, FLAG_RCV); break;}
            if ((frame[1] ^ frame[2]) == byteToProcess) {
                changeState(machine, BCC_OK);
                frame[3] = byteToProcess;
            }
            else changeState(machine, START);
            break;
        case BCC_OK:
            if (byteToProcess == FLAG) {changeState(machine, STOP); frame[4] = byteToProcess;}
            else changeState(machine, START);
            break;
        default:
            break;
        }
    }
    else if (mode == Info) {

    }
}

