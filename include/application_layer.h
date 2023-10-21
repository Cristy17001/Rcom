// Application layer protocol header.
// NOTE: This file must not be changed.

#ifndef _APPLICATION_LAYER_H_
#define _APPLICATION_LAYER_H_

#include "link_layer.h"
#include <string.h>

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

int buildControlPacket(const long int file_size, const char *filename, unsigned char name_size, unsigned char* frame, int isStart);
void buildDataPacket(short nDataBytes, unsigned char* packet, unsigned char* data);
int getDataSize(unsigned char* data);

#endif // _APPLICATION_LAYER_H_
