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
            if (byteToProcess == SET || byteToProcess == UA) {
                changeState(machine, C_RCV);
                frame[2] = byteToProcess;
            }
            else changeState(machine, START);
            break;
        case C_RCV:
            if (byteToProcess == FLAG) {changeState(machine, FLAG_RCV); break;}
            if (frame[1] ^ frame[2] == byteToProcess) {
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
    else if (mode == Data) {

    }
}

/*
int main() {
    StateMachine machine = {START};
    unsigned char frame[5] = {0};
    Mode mode = Communication;

    // Test Case 1: Valid frame
    unsigned char validFrame[] = {0x7E, 0x03, 0x03, 0x00, 0x7E};
    for (int i = 0; i < 5; i++) {
        StateHandler(&machine, validFrame[i], frame, mode);
    }
    if (machine.currentState == STOP) {
        printf("Test Case 1 Passed: Valid frame\n");
    } else {
        printf("Test Case 1 Failed: Valid frame\n");
    }

    // Test Case 2: Invalid address
    machine.currentState = START;
    unsigned char invalidAddressFrame[] = {0x7E, 0x02, 0x03, 0x01, 0x7E};
    for (int i = 0; i < 5; i++) {
        StateHandler(&machine, invalidAddressFrame[i], frame, mode);
    }
    if (machine.currentState == FLAG_RCV) {
        printf("Test Case 2 Passed: Invalid address\n");
    } else {
        printf("Test Case 2 Failed: Invalid address, and the current state is %d \n", machine.currentState);
    }

    // Test Case 3: Incorrect BCC
    machine.currentState = START;
    unsigned char incorrectBCCFrame[] = {0x7E, 0x03, 0x03, 0x01, 0x7E};
    for (int i = 0; i < 5; i++) {
        StateHandler(&machine, incorrectBCCFrame[i], frame, mode);
    }
    if (machine.currentState == FLAG_RCV) {
        printf("Test Case 3 Passed: Incorrect BCC\n");
    } else {
        printf("Test Case 3 Failed: Incorrect BCC\n");
    }

    return 0;
}
*/


