// Link layer protocol implementation

#include "link_layer.h"
#include <fcntl.h>
#include <termios.h>

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source
#define BUF_SIZE 5
////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters)
{
    volatile int STOP = FALSE;
    // Open serial port device for reading and writing and not as controlling tty
    // because we don't want to get killed if linenoise sends CTRL-C.
    int fd = open(connectionParameters.serialPort, O_RDWR | O_NOCTTY);
    if (fd < 0)
    {
        perror(connectionParameters.serialPort);
        exit(-1);
    }

    struct termios oldtio;
    struct termios newtio;

    // Save current port settings
    if (tcgetattr(fd, &oldtio) == -1)
    {
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
    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    printf("New termios structure set\n");
    unsigned char buf[BUF_SIZE] = {0};

    // Role of the Transmissor
    if (connectionParameters.role == LlTx) {
        // Send Set
        unsigned char buf[BUF_SIZE] = {0};

        buf[0] = FLAG;
        buf[1] = ADDRESS_1;
        buf[2] = SET;
        buf[3] = buf[1]^buf[2];
        buf[4] = FLAG;

        int bytes = write(fd, buf, BUF_SIZE);
        printf("Transmissor sending SET!\n");

        // Wait until all bytes have been written to the serial port
        sleep(1);

        // Receive UA
        while (STOP == FALSE)
        {
            // Returns after 5 chars have been input
            int bytes = read(fd, buf, BUF_SIZE);
            // Check if the "Trama" is correct
            if (buf[0] == FLAG && (buf[1]^buf[2] == buf[3] && buf[2] == UA) && buf[4] == FLAG) {
                printf("Transmissor confirmed UA reception!\n");
                // Connection confirmed
                STOP = TRUE;
            }
            
        }
    }

    // Role of the Receiver
    if (connectionParameters.role == LlRx) {
        // Receive Set

        // Send UA
    }

    // TODO

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
