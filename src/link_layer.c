// Link layer protocol implementation

#include "link_layer.h"

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source
#define BUF_SIZE 5

// Global variables
unsigned char mainFrame[MAX_PAYLOAD_SIZE];
int sizeMainFrame = MAX_PAYLOAD_SIZE;

int fd = 0;
int maxTries = 0;
int alarmEnabled = FALSE;


void SendMainFrame(int signal) {
    write(fd, mainFrame, sizeMainFrame);
    printf("Just sent mainFrame #%d!\n", maxTries);
    alarmEnabled = FALSE;
    maxTries++;
}

// Frame the SetFrame
void SetFrame() {
    mainFrame[0] = FLAG;
    mainFrame[1] = ADDRESS_1;
    mainFrame[2] = SET;
    mainFrame[3] = mainFrame[1]^mainFrame[2];
    mainFrame[4] = FLAG;
    sizeMainFrame = 5;
}

// Frame the SetFrame
void UAFrame() {
    mainFrame[0] = FLAG;
    mainFrame[1] = ADDRESS_1;
    mainFrame[2] = UA;
    mainFrame[3] = mainFrame[1]^mainFrame[2];
    mainFrame[4] = FLAG;
    sizeMainFrame = 5;
}

unsigned char PayloadBCC2(const unsigned char *buf, int bufSize) {
    // Payload BCC2
    unsigned char BCC2 = buf[0];
    for (int i = 1; i < bufSize; i++) {
        BCC2 ^= buf[i];
    }
    return BCC2;
}

unsigned char* Stuffing(const unsigned char *buf, int bufSize, int* newSize) {
    unsigned char* stuffedBuf = (unsigned char*)malloc(bufSize * 2 * sizeof(unsigned char));
    
    if (stuffedBuf == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        exit(1);
    }
    
    int stuffedIndex = 0;
    
    for (int i = 0; i < bufSize; i++) {
        if (buf[i] == 0x7e) {
            stuffedBuf[stuffedIndex++] = 0x7d;
            stuffedBuf[stuffedIndex++] = 0x5e;
        } else if (buf[i] == 0x7d) {
            stuffedBuf[stuffedIndex++] = 0x7d;
            stuffedBuf[stuffedIndex++] = 0x5d;
        } else {
            stuffedBuf[stuffedIndex++] = buf[i];
        }
    }
    
    stuffedBuf = realloc(stuffedBuf, stuffedIndex * sizeof(unsigned char));
    
    if (stuffedBuf == NULL) {
        fprintf(stderr, "Memory reallocation failed.\n");
        exit(1);
    }
    *newSize = stuffedIndex;
    
    return stuffedBuf;
}

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters) {
    fd = open(connectionParameters.serialPort, O_RDWR | O_NOCTTY);
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
        SetFrame(); // Place the set frame in the mainFrame
        (void)signal(SIGALRM, SendMainFrame);

        // UA confirmation auxiliar variables
        unsigned char byte_received[1] = {0};
        unsigned char UA_frame[BUF_SIZE] = {0};
        StateMachine UA_stateMachine = {START, UA};

        // RETRANSMIT MECHANISM AND UA CONFIRMATION
        while (maxTries < connectionParameters.nRetransmissions) {
            if (alarmEnabled == FALSE) {
                alarm(connectionParameters.timeout); // Set alarm to be triggered in "timeout" seconds
                printf("Sending Set Frame!\n");
                alarmEnabled = TRUE;
            }
            // Waiting for UA confirmation
            read(fd, &byte_received, 1);
            StateHandler(&UA_stateMachine, byte_received[0], UA_frame, Communication);

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
        unsigned char byte_received[1] = {0};
        unsigned char SET_frame[BUF_SIZE] = {0};
        StateMachine SET_stateMachine = {START, SET};
        printf("Waiting to receive SET!\n");

        while (TRUE) {
            // Waiting for SET confirmation
            int n_bytes = read(fd, &byte_received, 1);
            if (n_bytes > 0) {
                printf("Received byte: 0x%x\n", byte_received[0]);
            }
            StateHandler(&SET_stateMachine, byte_received[0], SET_frame, Communication);

            // SET frame received
            if (SET_stateMachine.currentState == STOP) {
                printf("SET frame received!\n");
                UAFrame();
                write(fd, mainFrame, sizeMainFrame);
                printf("UA frame sent!\n");
                return 0;
            }
        }
        
    }

    return 1;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(LinkLayer connectionParameters, const unsigned char *buf, int bufSize) {
    // Payload BCC2
    unsigned char BCC2 = PayloadBCC2(buf, bufSize);

    // Stuffing
    int newSize = 0;
    unsigned char* stuffedBuf = Stuffing(buf, bufSize, &newSize);

    // Populate frame
    mainFrame[0] = FLAG;
    mainFrame[1] = 0x01;
    mainFrame[2] = ADDRESS_1;
    mainFrame[3] = mainFrame[1] ^ mainFrame[2];
    for (int i = 0; i < newSize; i++) {
        mainFrame[i+4] = stuffedBuf[i];
    }
    mainFrame[newSize+4] = BCC2;
    mainFrame[newSize+5] = FLAG;
    sizeMainFrame = newSize+5;

    //ALARM SETUP
    (void)signal(SIGALRM, SendMainFrame);
    alarmEnabled = FALSE;
    maxTries = 0;

    unsigned char byte_received[1] = {0};

    // RETRANSMITION MECHANISM AND RR CONFIRMATION
    while (maxTries < connectionParameters.nRetransmissions) {
        if (alarmEnabled == FALSE) {
            alarm(connectionParameters.timeout); // Set alarm to be triggered in "timeout" seconds
            alarmEnabled = TRUE;
        }

        StateMachine RR_stateMachine = {START, RR};
        unsigned char* RR_frame = {0};
        StateMachine REJ_stateMachine = {START, REJ};
        unsigned char* REJ_frame = {0};

        // Waiting for RR confirmation
        int n_byte = read(fd, &byte_received, 1);
        if (n_byte > 0) {
            printf("Received byte: 0x%x\n", byte_received[0]);
        }

        StateHandler(&RR_stateMachine, byte_received[0], RR_frame, Communication);
        StateHandler(&REJ_stateMachine, byte_received[0], REJ_frame, Communication);


        if (RR_stateMachine.currentState == STOP) {
            printf("Positive ACK frame received!\n");
            free(stuffedBuf);
            return 0;
        }
        else if (REJ_stateMachine.currentState == STOP) {
            printf("Negative ACK frame received!\n");
            free(stuffedBuf);
            return 1;
        }
    }

    // Free dynamically allocated memory
    free(stuffedBuf);

    return 1;
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
