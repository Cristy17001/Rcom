// Application layer protocol implementation

#include "application_layer.h"

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename) {
    
    LinkLayerRole linkRole = (strcmp(role, "rx") == 0 ? LlRx : LlTx);
    char aux_serialPort[50];
    strcpy(aux_serialPort, serialPort);  // Copy the contents of serialPort to aux_serialPort
    LinkLayer connection_parameters = {0};  // Initialize the struct with all fields set to 0
    strcpy(connection_parameters.serialPort, aux_serialPort); // Copy aux_serialPort to serialPort field
    connection_parameters.role = linkRole;
    connection_parameters.baudRate = baudRate;
    connection_parameters.nRetransmissions = nTries;
    connection_parameters.timeout = timeout;

    // Establish connection with UA and SET
    if (llopen(connection_parameters) != 0) {
        printf("FAILED TO ESTABLISH CONNECTION!\n");
        exit(1);
    }

    if (connection_parameters.role == LlTx) {
        unsigned char test_buffer[11] = {0};

        for (int i = 0; i <= 9; i++) {
            test_buffer[i] =  (0x07 + i);
        }
        test_buffer[10] = 0x7e;
        llwrite(connection_parameters, test_buffer, 11);
    }
    else if (connection_parameters.role == LlRx) {
        unsigned char DataFrame[MAX_PAYLOAD_SIZE];
        if (llread(DataFrame) == -1) {
            printf("Some Error has Ocurred READING OR SENDING THAT DATA!\n");
        }
        else {
            printf("Correctly read the DATA!\n");
        }
    }


    if (llclose(connection_parameters, 0) != 0) {
        printf("FAILED TO TERMINATE CONNECTION!\n");
        exit(1);
    }
    
}

