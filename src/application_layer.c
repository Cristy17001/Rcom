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
        unsigned char frame[MAX_PAYLOAD_SIZE] = {0};
        unsigned char data[MAX_PAYLOAD_SIZE] = {0};
        FILE *file;
        int bytesRead;
        file = fopen(filename, "rb");

        if (!file) {
            perror("Error opening input file");
            exit(1);
        }

        fseek(file, 0, SEEK_END);
        const long size = ftell(file);
        fseek(file, 0, SEEK_SET);

        ///////////////////////////
        // START CONTROL PACKET  //
        ///////////////////////////
        int n_bytes = buildControlPacket(size, filename, 12, frame, TRUE);
        llwrite(connection_parameters, frame, n_bytes);

        ///////////////////////////
        // SENDING DATA PACKET   //
        ///////////////////////////
        while ((bytesRead = fread(data, sizeof(*data), MAX_PAYLOAD_SIZE-100, file)) > 0) {            
            buildDataPacket(bytesRead, frame, data);
            llwrite(connection_parameters, frame, bytesRead+3);            
        }

        ///////////////////////////
        // END CONTROL PACKET    //
        ///////////////////////////
        n_bytes = buildControlPacket(size, filename, 12, frame, FALSE);
        llwrite(connection_parameters, frame, n_bytes);

        fclose(file);
    }
    else if (connection_parameters.role == LlRx) {
        FILE *file_out;
        file_out = fopen(filename, "wb");
        if (!file_out) {
            perror("Error opening input file");
            exit(1);
        }

        unsigned char receivedData[MAX_PAYLOAD_SIZE*2];
        int response = 0;

        while (receivedData[0] != END_CTRL) {            
            response = llread(receivedData);

            if (response == -1) {
                printf("THIS PACKET SHOULD NOT BE SAVED, EITHER BECAUSE OF BCC2 ERROR OR PACKET REPETITION\n");
                continue;
            }
            else if (response >= 0) {
                printf("THIS PACKET SHOULD BE SAVED, THE BCC2 IS CORRECT AND THERE WAS NO REPETITION!\n");
                if (receivedData[0] == DATA_CTRL) {
                    int dataSize = getDataSize(receivedData);
                    fwrite(&receivedData[3], 1, dataSize, file_out);
                }

            }
        }
        fclose(file_out);
    }

    if (llclose(connection_parameters, 0) != 0) {
        printf("FAILED TO TERMINATE CONNECTION!\n");
        exit(1);
    }
}

int buildControlPacket(const long int file_size, const char *filename, unsigned char name_size, unsigned char* frame, int isStart) {
    long int file_size_aux = file_size;    
    int offset = 0;
    int counter_number_size = 0;

    frame[offset++] = (isStart ? START_CTRL : END_CTRL); // IS IT THE START CONTROL OR THE LAST CONTROL ? 
    frame[offset++] = 0; // SIZE OF THE FILE
    offset++;
    while (file_size_aux != 0) {
        frame[offset++] = (file_size_aux & 0xFF);
        file_size_aux = (file_size_aux >> 8); 
        counter_number_size++;
    }
    frame[2] = counter_number_size;  // Nº BYTES TO REPRESENT THE FILE_SIZE
    
    frame[offset++] = 1; // NAME OF THE FILE
    frame[offset++] = name_size; // Nº BYTES TO REPRESENT THE FILE_NAME
    
    for(int i = 0; i < name_size; i++) {
        frame[offset++] = filename[i];
    }

    return offset-1; // Return the number of bytes written in the frame
}

void buildDataPacket(int nDataBytes, unsigned char* packet, unsigned char* data) {
    packet[0] = DATA_CTRL;
    packet[1] = ((nDataBytes >> 8) & 0xff);
    packet[2] = nDataBytes & 0xff;
    for (int i = 0; i < nDataBytes; i++) {
        packet[3+i] = data[i]; 
    }
}

int getDataSize(unsigned char* data) {
    return 256 * data[1] + data[2];
}