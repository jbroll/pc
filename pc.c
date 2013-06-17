#include "pc.h"

extern int errno;
int alm_timeout();
int debug = 0;

#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
main(argc,argv)
int argc;
char *argv[];
{
	char dev[BUFSIZE];
	int speed;
	int timeout;
	int baudrate;
	int half = 0;
	int sync = 0;
	int verb = 0;

	int fd;
	char c;
	extern int optind;
	extern char *optarg;
	
	/* set the command line default variables */
	strcpy(dev,"/dev/ttyS1");
	speed = 38400;
	timeout = 10;

	/* check for command line variables */
	while ((c = getopt(argc, argv, "hd:s:D:xv")) != EOF){

		switch (c) {
			case 'h':	half = 1; break;
			case 'd':
				strcpy(dev,optarg);
				break;
	
			case 'x':	sync = 1; break;
			case 's':
				speed = atoi(optarg);
				break;
	
			case 'D':
				debug = atoi(optarg);
				break;
	
			case 'v': verb = 1;
				  break;

			case '?':
				fprintf(stderr,"\nusage: pc -[dsDv]");
				fprintf(stderr,"\n\t-d: set device [%s]",dev); 
				fprintf(stderr,"\n\t-s: set speed  [%d]",speed); 
				fprintf(stderr,"\n\t-D: set debug  [%d]",debug); 
				fprintf(stderr,"\n\t-v: print SCCS version"); 
				fprintf(stderr,"\n"); 
				fprintf(stderr,"\n[ ctrl-Y to kill ]\n\n");
				exit(1);
		}
	}

	/* debug default command line parameters */
	DEBUG( 1 ){
		fprintf(stderr,"\ndev=(%s)",dev);
		fprintf(stderr,"\nspeed=(%d)",speed);
		fprintf(stderr,"\nargc=(%d)",argc);
	}

	/* get ready to set the baudrate parameters */
	baudrate = getspeed( speed );

	/* open the tty */
	if ((fd = open(dev,O_RDWR)) < 0) {
		fprintf(stderr, "\npc: error on open; Can't open device: %s\n", dev);
		exit(1);
	}

	connect(fd,baudrate, half, sync, verb);
}
		
/*==================================================================*/
int getspeed( speed )
int speed;
{
int baudrate;

	switch(speed) {
		case 300:
			baudrate = B300;
			break;
		case 1200:
			baudrate = B1200;
			break;
		case 2400:
			baudrate = B2400;
			break;
		case 4800:
			baudrate = B4800;
			break;
		case 9600:
			baudrate = B9600;
			break;
		case 19200:
			baudrate = B19200;
			break;
		case 38400:
			baudrate = B38400;
			break;
		default:
			fprintf(stderr, "\npc: Bad speed: %s\n", speed);
			exit(1);
	}		

	return( baudrate );
}

/*=================================================================*/
connect(fd,baudrate, half, sync, verb)
int fd;
int baudrate, half, sync, verb;
{
	struct termios linesave, ctrlsave;
	int pid;
	static struct timeval tz;

	int	timeout = 1;
	char	waitfor[256];

	*waitfor = '\0';

	/* save the terminal and line states */
	tcgetattr(0,  &ctrlsave);
	tcgetattr(fd, &linesave);

	if ( !isatty(0) ) sync = 1;

	if ( sync ) {
			char	line[1024];
			int	n, c;
			char	rt = 13;
			struct termios tdescrip;
	
		tty_raw(fd,baudrate,1,0);

		while ( fgets(line, 1024, stdin) ) {
				 fd_set	readfds;

			n = strlen(line);

			if ( line[0] == '.' ) {
			    if ( n > 3 && strncmp(&line[2], "sleep",   5) )
				sleep(atoi(&line[8]));

			    if ( n > 3 && strncmp(&line[2], "timeout", 7) )
				timeout = atoi(&line[8]);
				
			    if ( n > 3 && strncmp(&line[2], "waitfor", 7) )
				strcpy(waitfor, &line[8]);

			    continue;
			}

			write(fd, line, n);
			write(fd, &rt, 1);
			if ( verb ) write(1, line, n);

			FD_ZERO(&readfds);
			FD_SET(fd, &readfds);
			tz.tv_sec  = timeout;
			tz.tv_usec = 500;
			for ( n = 0; select(FD_SETSIZE
					, &readfds, NULL, NULL
					, &tz)
				   ; n++ ) {
					int	nc, i;

			    do {
				nc = read(fd, line, 1024);
				for ( i = 0; i < nc; i++ )
					if ( line[i] == 13 ) line[i] = '\n';
				write(1, line, nc);

				if ( *waitfor && strcmp(waitfor, line) )
				    break;

				tz.tv_sec  =   0;
				tz.tv_usec = 500;
			    } while ( *waitfor );
			}
		}
	} else {
		/* set the terminals to raw states */
		tty_raw(0,0,1,0);
		tty_raw(fd,baudrate,1,0);

		/* fork the input and tty output */
		if( (pid = fork() ) != ERROR ){

			if(pid > 0) {
				/* start the send process */
				to(fd, half);
			}else{
				/* start the read process */
				from(fd);
			}
		}else{
			fprintf(stderr,"\n\nCould not fork process, die...");
		}

		/* kill the child process */
		kill(pid, SIGTERM);
		wait((int*)0);
	}

	/* restore terminal states */
	tcsetattr(0,  TCSANOW, &ctrlsave);
	tcsetattr(fd, TCSANOW, &linesave);
}

/*=================================================================*/
int tty_raw(fd,baudrate,min,time)
int fd;
int baudrate;
int min;
int time;
{
	struct termios tdescrip;

	if(!isatty(fd)){
		return 1;
	}

	/* get the current state of the terminals */
	if(tcgetattr(fd, &tdescrip) == ERROR ) return(ERROR);

	/* change required characteristics */
	tdescrip.c_iflag = (ISTRIP);
	tdescrip.c_oflag = 0;
	tdescrip.c_lflag = 0;
/*
	tdescrip.c_lflag &= ~ICANON;
*/
		
	if ( baudrate ) { 
		cfsetospeed(&tdescrip, baudrate);
		cfsetispeed(&tdescrip, baudrate);
	}

	tdescrip.c_cc[VMIN] = min;
	tdescrip.c_cc[VTIME] = time;
	
	return tcsetattr(fd, TCSANOW, &tdescrip );
}


/*=================================================================*/
to(fd, half)
	int fd, half;
{
	/* get chars from stdin and sends chars to the tty */
	unsigned char c;
	int n;

	for(;;){


		/* read from stdin */
		if ( (n = read(0, &c, 1)) < 1 ) return;


		if(c == '\04'){
			return EXIT;
		}else{
			if ( c == 10 || c == 13 ) {
				c = 10;
				if ( half ) n = write( 1, &c, 1);
				n = write(fd, &c, 1);
				c = 13;
			}
			if ( half ) n = write( 1, &c, 1);
			n = write(fd, &c, 1);
		}

		DEBUG(2){
			fprintf(stdout,"to: in=%d; %d",c,n);
			fflush(stdout);
		}
	}
}

/*=================================================================*/
int from(fd)
int fd;
{
	/* get chars from tty and send chars to the stdout */
	unsigned char c;
int n;
	for(;;){
		n = read(fd, &c, 1);
		if ( c == 10 || c == 13 ) {
			c = 10;
			n = write( 1, &c, 1);
			c = 13;
		}
		write(1, &c, 1);

		DEBUG(3){
			fprintf(stderr,"from: in=%d; %d",c,n);
			fflush(stderr);
		}
	}


}
 
