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
        llwrite(connection_parameters, test_buffer, 11);

        for (int i = 0; i <= 10; i++) {
            test_buffer[i] =  (0x02 + i);
        }
        llwrite(connection_parameters, test_buffer, 11);
    }
    else if (connection_parameters.role == LlRx) {
        unsigned char DataFrame[MAX_PAYLOAD_SIZE];
        int counter = 0;

        while (counter != 3) {
            int response = llread(DataFrame);

            if (response == -1) {
                printf("THIS PACKET SHOULD NOT BE SAVED, EITHER BECAUSE OF BCC2 ERROR OR PACKET REPETITION\n");
            }
            else if (response >= 0) {
                printf("THIS PACKET SHOULD BE SAVED, THE BCC2 IS CORRECT AND THERE WAS NO REPETITION!\n");
            }
            counter++;
        }
        
    }

    if (llclose(connection_parameters, 0) != 0) {
        printf("FAILED TO TERMINATE CONNECTION!\n");
        exit(1);
    }
    
}

void buildControlFrame(const long int file_size, unsigned char* name, unsigned char name_size, unsigned char* frame, int isStart) {
    long int file_size_aux = file_size;    
    int offset = 0;
    frame[0] = (isStart ? START_CTRL : END_CTRL); // IS IT THE START CONTROL OR THE LAST CONTROL ? 
    frame[1] = 0; // SIZE OF THE FILE
    frame[2] = sizeof(file_size);  // Nº BYTES TO REPRESENT THE FILE_SIZE

    while (offset < frame[2]) {
        frame[3+offset] = (file_size_aux & 0xFF);
        file_size_aux = (file_size_aux >> 8); 
        offset++;
    }
    offset += 4;
    frame[offset++] = 1; // NAME OF THE FILE
    frame[offset++] = sizeof(name_size); // Nº BYTES TO REPRESENT THE FILE_NAME
    while (offset < frame[offset-1]) {
        frame[offset++] = (file_size_aux & 0xFF);
        file_size_aux = (file_size_aux >> 8); 
    }
}