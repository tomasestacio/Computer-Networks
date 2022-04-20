//constantes hexadecimais
#ifndef CONSTANTS_H
#define CONSTANTS_H

#include "constants.h"
#include "linklayer.h"
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <unistd.h>

#define FLAG 0x7E
#define A_TRANS 0x03
#define A_REC 0x01
//C pode ser 0x02 ou 0x00 (information frame)
#define ESC 0x7D
#define SET 0x03
#define DISC 0x0B
#define UA 0x07


// Opens a conection using the "port" parameters defined in struct linkLayer, returns "-1" on error and "1" on sucess

speed_t get_baud(int baud);
int establishment_trans(); //returns 1 if everything goes well, -1 if error
int establishment_rec(); //returns 1 if everything goes well, -1 if error
unsigned char informationcheck(); //returns the control flag for the transmitter of the information packet
unsigned char confirmationcheck(); //returns the control flag for the receiver of the information packet
int transmitter_information_write(char* buf, int bufSize); //stuffing included, returns size of buffer if everything goes ok, if error returns 0
int transmitter_information_read(); //returns 1 if everything is ok, 0 if REJ was sent and -1 if timeout was activated during the read
int receiver_information_read(); //returns size of packet if everything is ok, 0 if REJ was sent and -1 if timeout was activated during the read
int receiver_information_write(char* packet); //returns 1, if error returns 0
int termination_trans(); //returns 1 if everything goes well, -1 if error
int termination_rec(); //returns 1 if everything goes well, -1 if error

#endif
