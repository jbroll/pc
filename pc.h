
#include <stdio.h>
#include <fcntl.h>
#include <termios.h>
#include <signal.h>
#include <setjmp.h>

#define ERROR (-1)
#define SUCESS 0
#define TRUE 1
#define FALSE 0
#define SUCCESS 0
#define EXIT 2

#define MAXTIME 30

#define CTRLD 004
#define CTRLP 020
#define CTRLY 031

#define TIMEOUT 10

#define DEBUG(x)	if( debug == x )
#define BUFSIZE 256



