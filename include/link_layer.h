// Link layer header.
// NOTE: This file must not be changed.

#ifndef _LINK_LAYER_H_
#define _LINK_LAYER_H_

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>
#include "stateMachine.h"
#include "global_macros.h"

typedef enum
{
    LlTx,
    LlRx,
} LinkLayerRole;

typedef struct
{
    char serialPort[50];
    LinkLayerRole role;
    int baudRate;
    int nRetransmissions;
    int timeout;
} LinkLayer;

// SIZE of maximum acceptable payload.
// Maximum number of bytes that application layer should send to link layer
#define MAX_PAYLOAD_SIZE 1000

/**
 * @brief Restarts the variables responsible for controlling the alarm.
 */
void restartAlarm();

/**
 * @brief Sends the main frame through the cable.
 */
void SendMainFrame();

/**
 * @brief Handles the alarm and counts time.
 * 
 * @param signal The signal triggering the alarm handler.
 */
void alarmHandler(int signal);

/**
 * @brief Places the set frame into the main frame.
 */
void SetFrame();

/**
 * @brief Places the UA frame into the main frame.
 * 
 * @param address The address to be included in the frame.
 */
void UAFrame(int address);

/**
 * @brief Places the DISC frame into the main frame.
 * 
 * @param address The address to be included in the frame.
 */
void DiscFrame(int address);

/**
 * @brief Places the RR and REJ frames into the main frame.
 * 
 * @param control The control byte for the frame.
 */
void DataResponseFrame(unsigned char control);

/**
 * @brief Copies data from one frame to another.
 * 
 * @param source The source frame.
 * @param destination The destination frame.
 * @param size The size of the data to be copied.
 */
void copyDataFrame(unsigned char* source, unsigned char* destination, int size);

/**
 * @brief Payloads the BCC2 and performs stuffing.
 * 
 * @param buf The buffer containing data.
 * @param bufSize The size of the buffer.
 * @param BCC2 The BCC2 array to be populated.
 * @return int Returns 0 on success, or an error code on failure.
 */
int PayloadBCC2(const unsigned char *buf, int bufSize, unsigned char BCC2[2]);

/**
 * @brief Performs data stuffing and returns the stuffed data.
 * 
 * @param buf The buffer containing data.
 * @param bufSize The size of the buffer.
 * @param newSize Pointer to store the size of the new, stuffed data.
 * @return unsigned char* Pointer to the stuffed data.
 */
unsigned char* Stuffing(const unsigned char *buf, int bufSize, int* newSize);


// Open a connection using the "port" parameters defined in struct linkLayer.
// Return "0" on success or "1" on error.
int llopen(LinkLayer connectionParameters);

// Send data in buf with size bufSize.
// Return 0 on sucess, or "1" on error.
int llwrite(LinkLayer connectionParameters, const unsigned char *buf, int bufSize);

// Receive data in packet.
// Return 0 on sucess, or "1" on error.
int llread(LinkLayer connectionParameters, unsigned char *packet);

// Close previously opened connection.
// if showStatistics == TRUE, link layer should print statistics in the console on close.
// Return "1" on success or "-1" on error.
int llclose(LinkLayer connectionParameters, int showStatistics);

#endif // _LINK_LAYER_H_
