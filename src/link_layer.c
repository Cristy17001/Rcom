// Link layer protocol implementation

#include "link_layer.h"

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source
#define BUF_SIZE 5

int alarmEnabled = FALSE;
int maxTries = 0;

// Alarm function handler's
void SendSet(int signal, int fd) {
    // Send Set
    unsigned char SET_frame[BUF_SIZE] = {0};
    SET_frame[0] = FLAG;
    SET_frame[1] = ADDRESS_1;
    SET_frame[2] = SET;
    SET_frame[3] = SET_frame[1]^SET_frame[2];
    SET_frame[4] = FLAG;

    int bytes = write(fd, SET_frame, BUF_SIZE);
    printf("Emissor just sent SET #%d!\n", maxTries);
    printf("Waiting for UA confirmation!\n");  

    alarmEnabled = FALSE;
    maxTries++;
}

void SendUA(int fd) {
    // Send UA
    unsigned char UA_frame[BUF_SIZE] = {0};
    UA_frame[0] = FLAG;
    UA_frame[1] = ADDRESS_1;
    UA_frame[2] = UA;
    UA_frame[3] = UA_frame[1]^UA_frame[2];
    UA_frame[4] = FLAG;

    int bytes = write(fd, UA_frame, BUF_SIZE);
    printf("Recepter just sent UA!\n");
}

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters) {
    int fd = open(connectionParameters.serialPort, O_RDWR | O_NOCTTY);
    if (fd < 0) {
        perror(connectionParameters.serialPort);
        exit(-1);
    }

    struct termios oldtio;
    struct termios newtio;

    // Save current port settings
    if (tcgetattr(fd, &oldtio) == -1) {
        perror("tcgetattr");
        exit(-1);
    }

    // Clear struct for new port settings
    memset(&newtio, 0, sizeof(newtio));

    newtio.c_cflag = connectionParameters.baudRate | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    // Set input mode (non-canonical, no echo,...)
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 0; // Inter-character timer unused
    newtio.c_cc[VMIN] = 1;  // Blocking read until 1 chars received

    // TCIFLUSH - flushes data received but not read.
    tcflush(fd, TCIOFLUSH);

    // Set new port settings
    if (tcsetattr(fd, TCSANOW, &newtio) == -1) {
        perror("tcsetattr");
        exit(-1);
    }
    printf("Port configuration step done!\n");

    // Emissor
    if (connectionParameters.role == LlTx) {
        // SET UP THE ALARM
        (void)signal(SIGALRM, SendSet);

        // UA confirmation auxiliar variables
        unsigned char byte_received = 0;
        unsigned char UA_frame[BUF_SIZE] = {0};
        StateMachine UA_stateMachine = {START};

        // RETRANSMIT MECHANISM AND UA CONFIRMATION
        while (maxTries < connectionParameters.nRetransmissions) {
            if (alarmEnabled == FALSE) {
                alarm(connectionParameters.timeout); // Set alarm to be triggered in "timeout" seconds
                alarmEnabled = TRUE;
            }
            // Waiting for UA confirmation
            read(fd, byte_received, 1);
            StateHandler(&UA_stateMachine, byte_received, UA_frame, Communication);

            // UA frame received and connection is now established
            if (UA_stateMachine.currentState == STOP) {
                printf("UA frame received!\n");
                return 0;
            }
        }
    }

    // Role of the Receiver
    if (connectionParameters.role == LlRx) {
        // SET confirmation auxiliar variables
        unsigned char byte_received = 0;
        unsigned char SET_frame[BUF_SIZE] = {0};
        StateMachine SET_stateMachine = {START};

        // Waiting for SET confirmation
        read(fd, byte_received, 1);
        StateHandler(&SET_stateMachine, byte_received, SET_frame, Communication);

        // SET frame received
        if (SET_stateMachine.currentState == STOP) {
            printf("SET frame received!\n");
            SendUA(fd);
            printf("UA frame sent!\n");
            return 0;
        }
    }

    return 1;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize)
{
    // TODO

    return 0;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    // TODO

    return 0;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics)
{
    // TODO

    return 1;
}
