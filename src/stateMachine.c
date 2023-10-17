#include "stateMachine.h"

void changeState(StateMachine *machine, State state) {
    machine->currentState = state;
}

// Function to handle changes in states that returns the current index of the frame
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
            if (byteToProcess == (machine->addressNumber == 1 ? ADDRESS_1 : ADDRESS_2)) {
                changeState(machine, A_RCV);
                frame[1] = byteToProcess;
                machine->frameSize = 1;
            }
            else changeState(machine, START);
            break;
        case A_RCV:
            if (byteToProcess == FLAG) {changeState(machine, FLAG_RCV); break;}
            if (byteToProcess == machine->controlByte) {
                changeState(machine, C_RCV);
                frame[2] = byteToProcess;
                machine->frameSize = 2;
            }
            else changeState(machine, START);
            break;
        case C_RCV:
            if (byteToProcess == FLAG) {changeState(machine, FLAG_RCV); break;}
            if ((frame[1] ^ frame[2]) == byteToProcess) {
                changeState(machine, BCC_OK);
                frame[3] = byteToProcess;
                machine->frameSize = 3;
            }
            else changeState(machine, START);
            break;
        case BCC_OK:
            if (byteToProcess == FLAG) {changeState(machine, STOP); frame[4] = byteToProcess; machine->frameSize = 4;}
            else changeState(machine, START);
            break;
        default:
            break;
        }
    }
    else if (mode == Data) {
        switch (machine->currentState) {
        case START:
            if (byteToProcess == FLAG) {
                changeState(machine, FLAG_RCV);
                frame[0] = byteToProcess;
                machine->frameSize = 0;
            }
            break;
        case FLAG_RCV:
            if (byteToProcess == FLAG) break;
            if (byteToProcess == (machine->addressNumber == 1 ? ADDRESS_1 : ADDRESS_2)) {
                changeState(machine, A_RCV);
                frame[1] = byteToProcess;
                machine->frameSize = 1;
            }
            else changeState(machine, START);
            break;
        case A_RCV:
            if (byteToProcess == FLAG) {changeState(machine, FLAG_RCV); break;}
            if (byteToProcess == machine->controlByte) {
                changeState(machine, C_RCV);
                frame[2] = byteToProcess;
                machine->frameSize = 2;
            }
            else changeState(machine, START);
            break;
        case C_RCV:
            if (byteToProcess == FLAG) {changeState(machine, FLAG_RCV); break;}
            if ((frame[1] ^ frame[2]) == byteToProcess) {
                changeState(machine, BCC_OK);
                frame[3] = byteToProcess;
                machine->frameSize = 3;
            }
            else changeState(machine, START);
            break;
        case BCC_OK:
            if (byteToProcess == FLAG) {
                frame[++(machine->frameSize)] = byteToProcess;
                changeState(machine, STOP);
            }
            else {
                frame[++(machine->frameSize)] = byteToProcess;
            }
            break;
        default:
            break;
        }
    }
}

