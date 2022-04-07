//constantes hexadecimais
#define FLAG 0x7E
#define A_TRANS 0x03
#define A_REC 0x01
//C pode ser 0x02 ou 0x00 (information frame)
#define ESC 0x7D
#define SET 0x03
#define DISC 0x0B
#define UA 0x07


// Opens a conection using the "port" parameters defined in struct linkLayer, returns "-1" on error and "1" on sucess

int get_baud(int baud);
int establishment_trans(); //returns 1 if everything goes well, -1 if error -> include in llwrite
int establishment_rec(); //returns 1 if everything goes well, -1 if error -> include in llread
