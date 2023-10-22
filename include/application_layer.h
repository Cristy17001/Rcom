// Application layer protocol header.
// NOTE: This file must not be changed.

#ifndef _APPLICATION_LAYER_H_
#define _APPLICATION_LAYER_H_

#include "link_layer.h"
#include <string.h>
#include <stdlib.h>

// Application layer main function.
// Arguments:
//   serialPort: Serial port name (e.g., /dev/ttyS0).
//   role: Application role {"tx", "rx"}.
//   baudrate: Baudrate of the serial port.
//   nTries: Maximum number of frame retries.
//   timeout: Frame timeout.
//   filename: Name of the file to send / receive.
void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename);

/**
 * @brief Builds the control packet with start or end information.
 * 
 * @param file_size The size of the file.
 * @param filename The name of the file.
 * @param name_size The size of the filename.
 * @param frame The buffer to hold the constructed packet.
 * @param isStart Flag indicating whether it's a start packet (1) or end packet (0).
 * @return int Return the number of bytes written in the frame
 */
int buildControlPacket(const long int file_size, const char *filename, unsigned char name_size, unsigned char* frame, int isStart);

/**
 * @brief Builds a data packet with the specified number of data bytes.
 * 
 * @param nDataBytes The number of data bytes.
 * @param packet The buffer to hold the constructed data packet.
 * @param data The data to be included in the packet.
 */
void buildDataPacket(int nDataBytes, unsigned char* packet, unsigned char* data);

/**
 * @brief Returns the data size based on the MSB (L2) and LSB (L1) of the data.
 * 
 * @param data The data for which size is to be determined.
 * @return int The calculated data size.
 */
int getDataSize(unsigned char* data);


#endif // _APPLICATION_LAYER_H_
