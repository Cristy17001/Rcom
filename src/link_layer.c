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


void restartAlarm() {
    maxTries = 0;
    alarmEnabled = FALSE;
}

void SendMainFrame(int signal) {
    write(fd, mainFrame, sizeMainFrame);
    printf("Sent mainFrame #%d: \n", maxTries);
    for (int i = 0; i < sizeMainFrame; i++) {
        printf("0x%x ", mainFrame[i]);
    }
    printf("\n\n");
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

// Frame the UAFrame
void UAFrame(int address) {
    mainFrame[0] = FLAG;
    mainFrame[1] = (address == 1 ? ADDRESS_1 : ADDRESS_2);
    mainFrame[2] = UA;
    mainFrame[3] = mainFrame[1]^mainFrame[2];
    mainFrame[4] = FLAG;
    sizeMainFrame = 5;
}

// Frame the DiscFrame
void DiscFrame(int address) {
    mainFrame[0] = FLAG;
    mainFrame[1] = (address == 1 ? ADDRESS_1 : ADDRESS_2);
    mainFrame[2] = DISC;
    mainFrame[3] = mainFrame[1]^mainFrame[2];
    mainFrame[4] = FLAG;
    sizeMainFrame = 5;
}

int PayloadBCC2(const unsigned char *buf, int bufSize, unsigned char BCC2[2]) {
    // Payload BCC2
    BCC2[0] = buf[0];
    for (int i = 1; i < bufSize; i++) {
        BCC2[0] ^= buf[i];
    }


    // Stuffing BCC2 locally
    if (BCC2[0] == 0x7e) {
        BCC2[0] = 0x7d;
        BCC2[1] = 0x5e;
        
    }
    else if (BCC2[0] == 0x7d){
        BCC2[0] = 0x7d;
        BCC2[1] = 0x5d;
    }
    
    return 0;
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
    printf("\nPORT CONFIG DONE!\n\n");

    // ALARM SETUP
    (void)signal(SIGALRM, SendMainFrame);

    // Emissor
    if (connectionParameters.role == LlTx) {
        SetFrame(); // Place the set frame in the mainFrame

        // UA confirmation auxiliar variables
        unsigned char byte_received[1] = {0};
        unsigned char UA_frame[BUF_SIZE] = {0};
        StateMachine UA_stateMachine = {START, UA, 1};

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
                printf("UA frame received!\n\n");
                return 0;
            }
        }
    }

    // Role of the Receiver
    if (connectionParameters.role == LlRx) {
        // SET confirmation auxiliar variables
        unsigned char byte_received[1] = {0};
        unsigned char SET_frame[BUF_SIZE] = {0};
        StateMachine SET_stateMachine = {START, SET, 1};
        printf("WAITING SET FRAME!\n");

        while (TRUE) {
            // Waiting for SET confirmation
            int n_bytes = read(fd, &byte_received, 1);
            if (n_bytes > 0) {
                printf("0x%x \n", byte_received[0]);
            }
            StateHandler(&SET_stateMachine, byte_received[0], SET_frame, Communication);

            // SET frame received
            if (SET_stateMachine.currentState == STOP) {
                printf("SET FRAME RECEIVED!\n");
                UAFrame(1);
                printf("SENDING UA FRAME!\n");
                write(fd, mainFrame, sizeMainFrame);
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
    unsigned char BCC2[2] = {0};
    PayloadBCC2(buf, bufSize, BCC2);

    // Stuffing
    int newSize = 0;
    unsigned char* stuffedBuf = Stuffing(buf, bufSize, &newSize);

    // Populate frame
    mainFrame[0] = FLAG;
    mainFrame[1] = ADDRESS_1;
    mainFrame[2] = 0;
    mainFrame[3] = mainFrame[1] ^ mainFrame[2];
    for (int i = 0; i < newSize; i++) {
        mainFrame[i+4] = stuffedBuf[i];
    }
    mainFrame[newSize+4] = BCC2[0];
    if (BCC2[1] == 0x5e || BCC2[1] == 0x5d) {
        mainFrame[newSize+5] = BCC2[1];
        mainFrame[newSize+6] = FLAG;
        sizeMainFrame = newSize+7;
    }
    else {
        mainFrame[newSize+5] = FLAG;
        sizeMainFrame = newSize+6;
    }


    restartAlarm();

    unsigned char byte_received[1] = {0};

    printf("WAITING CONFIRMATION OR REJECTION!\n");
    // RETRANSMITION MECHANISM AND RR CONFIRMATION
    while (maxTries < connectionParameters.nRetransmissions) {
        if (alarmEnabled == FALSE) {
            alarm(connectionParameters.timeout); // Set alarm to be triggered in "timeout" seconds
            alarmEnabled = TRUE;
        }

        StateMachine RR_stateMachine = {START, RR, 1};
        unsigned char* RR_frame = {0};
        StateMachine REJ_stateMachine = {START, REJ, 1};
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
int llread(unsigned char *packet) {
    StateMachine dataMachine = {START, 0, 1};
    unsigned char byte_received[1] = {0};
    unsigned char dataFrame[MAX_PAYLOAD_SIZE * 2] = {0}; 
    int frame_index = 0;
    
    // Process the data received until correct frame structure works
    while (dataMachine.currentState != STOP) {
        read(fd, &byte_received, 1);

        // Process the data received and return the index of the current frame
        frame_index = StateHandler(&dataMachine, byte_received[0], dataFrame, Data);
    }

    printf("DATA PACKET RECEIVED: \n");
    for (int i = 0; i <= frame_index; i++) {
        printf("0x%x ", dataFrame[i]);
    }
    printf("\n\n");

    unsigned char *dataDestuffed = (unsigned char *)malloc(MAX_PAYLOAD_SIZE * 2 * sizeof(unsigned char));

    // Destuffing of the DATA and BBC2
    int dD_index = 0; // Data destuffed index
    int skip_index = 0; // Used to skip character for the special cases
    while (skip_index < frame_index-4) {
        if (dataFrame[skip_index+4] == 0x7d) {
            if (dataFrame[skip_index+5] == 0x5e) {
                dataDestuffed[dD_index] = 0x7e;
                skip_index += 2;
                dD_index++;
                continue;
            }
            else if (dataFrame[skip_index+5] == 0x5d) {
                dataDestuffed[dD_index] = 0x7d;
                skip_index += 2;
                dD_index++;
                continue;
            }
        }
        dataDestuffed[dD_index] = dataFrame[skip_index+4];
        skip_index++;
        dD_index++;
    }

    printf("Data Destuffed: \n");
    for (int i = 0; i < dD_index; i++) {
        printf("0x%x ", dataDestuffed[i]);
    }
    printf("\n\n");

    // Verify BCC2 validaty
    unsigned char checkBCC2 = dataDestuffed[0];
    for (int i = 1; i < dD_index-1; i++) {
        printf("0x%02x XOR 0x%02x = ", checkBCC2, dataDestuffed[i]);
        checkBCC2 ^= dataDestuffed[i];
        printf("0x%02x\n", checkBCC2);
    }

    printf("\ndD_index: %d\n", dD_index);
    printf("0x%02x\n", dataDestuffed[dD_index-1]);
    printf("0x%02x\n", checkBCC2);
    // VALID BCC2
    if (dataDestuffed[dD_index-1] == checkBCC2) {
        packet = realloc(dataDestuffed, dD_index * sizeof(unsigned char));
        return dD_index;
    }
    
    return -1;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(LinkLayer connectionParameters, int showStatistics) {
    unsigned char buf[BUF_SIZE] = {0};
    unsigned char byte_received[1] = {0};

    if (connectionParameters.role == LlTx) {
        restartAlarm();
        DiscFrame(1);
        StateMachine DISC_stateMachine = {START, DISC, 2};
        while (maxTries < connectionParameters.nRetransmissions) {
            if (alarmEnabled == FALSE) {
                alarm(connectionParameters.timeout); // Set alarm to be triggered in "timeout" seconds
                printf("Sending DISC Frame!\n");
                alarmEnabled = TRUE;
            }
            // Waiting for DISC confirmation
            read(fd, &byte_received, 1);
            StateHandler(&DISC_stateMachine, byte_received[0], buf, Communication);

            // DISC frame received, sending UA
            if (DISC_stateMachine.currentState == STOP) {
                printf("DISC frame received!\n");
                printf("Sending UA frame!\n");

                // Sending UA
                UAFrame(2);
                write(fd, mainFrame, sizeMainFrame);
                return 0;
            }
        }
    }
    else if (connectionParameters.role == LlRx) {
        StateMachine DISC_stateMachine = {START, DISC, 1};
        StateMachine UA_stateMachine = {START, UA, 2};

        while (TRUE) {
            read(fd, &byte_received, 1);
            StateHandler(&DISC_stateMachine, byte_received[0], buf, Communication);

            // DISC frame received, sending DISC
            if (DISC_stateMachine.currentState == STOP) {
                printf("DISC frame received!\n");
                printf("Sending DISC frame!\n");
                break;
            }
        }
        
        DiscFrame(2);
        restartAlarm();
        while (maxTries < connectionParameters.nRetransmissions) {
            if (alarmEnabled == FALSE) {
                alarm(connectionParameters.timeout); // Set alarm to be triggered in "timeout" seconds
                printf("Sending DISC Frame!\n");
                alarmEnabled = TRUE;
            }
            read(fd, &byte_received, 1);
            StateHandler(&UA_stateMachine, byte_received[0], buf, Communication);

            if (UA_stateMachine.currentState == STOP) {
                printf("UA frame received!\n");
                printf("Seizing all communication!\n");
                return 0;
            }
        }
    }
    printf("Some error occurred seizing communication!\n");
    return 1;
}
