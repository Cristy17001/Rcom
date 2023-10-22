// Application layer protocol implementation

#include "application_layer.h"

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename) {
    
    // INITIALIZE LAYER CONNECTION VARIABLE
    LinkLayerRole linkRole = strcmp(role, "rx") == 0 ? LlRx : LlTx;
    LinkLayer connection_parameters = { .role = linkRole, .baudRate = baudRate, .nRetransmissions = nTries, .timeout = timeout };
    snprintf(connection_parameters.serialPort, sizeof(connection_parameters.serialPort), "%s", serialPort);


    // ESTABLISH CONNECTION WITH SET AND UA
    if (llopen(connection_parameters) != 0) {
        printf("FAILED TO ESTABLISH CONNECTION!\n");
        exit(1);
    }

    // TRANSMISSOR
    if (connection_parameters.role == LlTx) {
        unsigned char frame[MAX_PAYLOAD_SIZE] = {0};
        unsigned char data[MAX_PAYLOAD_SIZE] = {0};
        FILE *file;
        int bytesRead = 1;
        file = fopen(filename, "rb");

        if (!file) {
            perror("Error opening input file");
            exit(1);
        }

        // GET THE NUMBER OF BYTES IN THE FILE
        fseek(file, 0, SEEK_END);
        const long size = ftell(file);
        fseek(file, 0, SEEK_SET);

        ///////////////////////////
        // START CONTROL PACKET  //
        ///////////////////////////
        int n_bytes = buildControlPacket(size, filename, strlen(filename), frame, TRUE);
        if (llwrite(connection_parameters, frame, n_bytes) == 1) {
            printf("ERROR: MAXIMUM RETRANSMISSIONS EXCEEDED!");
            fclose(file);
            exit(1);
        }

        ///////////////////////////
        // SENDING DATA PACKET   //
        ///////////////////////////
        while (bytesRead > 0) {        
            bytesRead = fread(data, sizeof(*data), MAX_PAYLOAD_SIZE-50, file);    
            buildDataPacket(bytesRead, frame, data);
            if (llwrite(connection_parameters, frame, bytesRead+3) == 1) {
                printf("ERROR: MAXIMUM RETRANSMISSIONS EXCEEDED!");
                fclose(file);
                exit(1);
            }            
        }

        ///////////////////////////
        // END CONTROL PACKET    //
        ///////////////////////////
        n_bytes = buildControlPacket(size, filename, strlen(filename), frame, FALSE);
        if (llwrite(connection_parameters, frame, n_bytes) == 1) {
            printf("ERROR: MAXIMUM RETRANSMISSIONS EXCEEDED!");
            fclose(file);
            exit(1);
        }
        fclose(file);
    }
    // RECEIVER
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
            response = llread(connection_parameters, receivedData);

            if (response == -1) {
                printf("THIS PACKET SHOULD NOT BE SAVED, EITHER BECAUSE OF BCC2 ERROR OR PACKET REPETITION\n\n");
                continue;
            }
            else if (response >= 0) {
                printf("THIS PACKET SHOULD BE SAVED, THE BCC2 IS CORRECT AND THERE WAS NO REPETITION!\n\n");
                if (receivedData[0] == DATA_CTRL) {
                    int dataSize = getDataSize(receivedData);
                    fwrite(&receivedData[3], 1, dataSize, file_out);
                }

            }
        }
        fclose(file_out);
    }

    // CLOSE CONNECTION WITH DISC FROM TX, DISC FROM RX AND UA FROM TX
    if (llclose(connection_parameters, FALSE) != 0) {
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