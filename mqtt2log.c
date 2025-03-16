/*                               -*- Mode: C -*- 
 * Copyright (C) 2018-2024, Mats Bergstrom
 * 
 * File name       : mqtt2log.c
 * Description     : mqtt2log for mqtt events to text file.
 * 
 * Author          : Mats Bergstrom
 * Created On      : Tue Oct 30 18:02:22 2018
 * 
 */

/*
 * Topics logged:
 *
 * hus/test		: # <payload>
 * han/raw		: not logged.
 * han/#		: H <topic> <payload>
 * hus/vatten/abs	: V <payload>
 * hus/fv/abs		: F <payload>
 * hus/TH/#		: T <payload>
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#include <mosquitto.h>

#include "cfgf.h"



/* -------------------------------------------------------------------------- */
/* Configuration */

char* mqtt_broker = 0;
int   mqtt_port;
char* mqtt_id = 0;

size_t logfile_max = 100000;


int
set_broker( int argc, const char** argv)
{
    if ( argc != 4 )
	return -1;
    
    if (mqtt_broker)
	free( mqtt_broker );
    mqtt_broker = strdup( argv[1] );
    if ( !mqtt_broker || !*mqtt_broker )
	return -1;

    mqtt_port = atoi(argv[2]);
    if ( (mqtt_port < 1) || (mqtt_port > 65535) )
	return -1;

    if (mqtt_id)
	free( mqtt_id );
    mqtt_id = strdup( argv[1] );
    if ( !mqtt_id || !*mqtt_id )
	return -1;

    return 0;
}



int
set_logfile( int argc, const char** argv)
{
    if ( argc != 2 )
	return -1;
    logfile_max = atol(argv[1]);
    if ( logfile_max < 100 )
	return -1;
    return 0;
}

int
set_logfile_max( int argc, const char* argv)
{
    if ( argc != 2 )
	return -1;
    return 0;
}


typedef struct {
    const char* topic;
    const char* metric;
    const char* tagval;
} trans_t;

#define MAX_TOPIC	(256)
unsigned n_ttab = 0;
trans_t ttab[ MAX_TOPIC ] = { {0,0,0} };

#define MAX_SUBSCRIBE	(32)
unsigned n_sub_str = 0;
char* sub_str[ MAX_SUBSCRIBE ] = { 0 };


int
match(const char* topic)
{
    int i = 0;
    while( ttab[i].topic ) {
	if ( !strcmp(ttab[i].topic,topic) )
	    return i;
	++i;
    }
    return -1;
}



int
set_subscribe( int argc, const char** argv )
{
    char* s = 0;
    
    if ( argc != 2 )
	return -1;

    if ( n_sub_str >= MAX_SUBSCRIBE-1 ) {
	printf("Too many subscribe.\n");
	return -1;
    }

    s = strdup ( argv[1] );
    if ( !s || !*s )
	return -1;
    sub_str[ n_sub_str ] = s;
    ++n_sub_str;
    sub_str[ n_sub_str ] = 0;
    
    /* Replace any * with # */
    while ( *s ) {
	if ( *s == '*' )
	    *s = '#';
	++s;
    }

    return 0;
}

int
set_topic(int argc, const char** argv)
{
    if ( argc != 4 )
	return -1;

    
    if ( n_ttab >= MAX_TOPIC-2 ) {
	printf("Too many topics!\n");
	return -1;
    }

    ttab[ n_ttab ].topic  = strdup( argv[1] );
    if ( !strcmp("@ignore", argv[2]) ) {
	ttab[ n_ttab ].metric = 0;
	ttab[ n_ttab ].tagval = 0;
    }
    else {
	ttab[ n_ttab ].metric = strdup( argv[2] );
	ttab[ n_ttab ].tagval = strdup( argv[3] );    

	if ( !ttab[ n_ttab ].topic ||
	     !ttab[ n_ttab ].metric ||
	     !ttab[ n_ttab ].tagval ) {
	    printf("Failed to create topic!\n");
	    return -1;
	}
    }
    ++n_ttab;

    ttab[ n_ttab ].topic  = 0;
    ttab[ n_ttab ].metric = 0;
    ttab[ n_ttab ].tagval = 0;


    return 0;
}


cfgf_tagtab_t tagtab[] = {
			  {"mqtt",	3, set_broker },
			  {"logfile",	1, set_logfile },
			  {"subscribe",	1, set_subscribe },
			  {"topic",	3, set_topic },
			  {0,0,0}
};




/* -------------------------------------------------------------------------- */
/* logfile handling */

FILE* lf = 0;				/* Log file */
unsigned lf_n = 0;			/* No of lines in log file */

#define LF_BLEN	(512)
char	lf_buf[ LF_BLEN ];
unsigned lf_len = 0;


void
lf_open()
{
    char fnam[64];
    time_t t;
    struct tm* tmp;
    
    t = time(0);
    tmp = localtime(&t);

    sprintf(fnam, "%04d-%02d-%02d:%02d:%02d:%02d.log",
	    tmp->tm_year + 1900,
	    tmp->tm_mon+1,
	    tmp->tm_mday,
	    tmp->tm_hour,
	    tmp->tm_min,
	    tmp->tm_sec );
	    
    
    lf = fopen( fnam, "a" );
    if ( !lf ) {
	perror("fopen: ");
	exit( EXIT_FAILURE );
    }
    setbuf(lf,0);

    lf_n = 0;

    /* print fixvalues */
}


void
lf_close()
{
    if ( lf ) {
	fclose( lf );
	lf = 0;
    }
}


void
lf_put(const char* s)
{
    fputs( s, lf );
    ++lf_n;
    if ( lf_n > logfile_max ) {
	lf_close();
	lf_open();
    }

}


void
lf_start(char c)
{
    struct timespec ts;
    int i;
    i = clock_gettime(CLOCK_REALTIME, &ts);
    if ( i ) {
	perror("clock_gettime: ");
	exit( EXIT_FAILURE );
    }

    lf_len = 0;

    sprintf(lf_buf, "%c %12ld.%03ld ", c, ts.tv_sec, ts.tv_nsec / 1000000 );
    lf_len = 12+1+3+1+1+1;
    lf_buf[lf_len] = '\0';
}


void
lf_end()
{
    lf_buf[lf_len] = '\n'; ++lf_len;
    lf_buf[lf_len] = '\0';
    fputs( lf_buf, lf );
    ++lf_n;
    if ( lf_n > logfile_max ) {
	lf_close();
	lf_open();
    }
}


void
lf_addl(const char* s, size_t s_len)
{
    if ( s_len ) {
	strncpy( lf_buf + lf_len, s, s_len );
	lf_len += s_len;
	lf_buf[lf_len] = '\0';
    }
}

void
lf_add(const char* s)
{
    unsigned s_len = strlen(s);
    lf_addl(s,s_len);
}


void
lf_log_s( const char* s )
{
    lf_start('L');
    lf_add(s);
    lf_end();
    /*
    fprintf( lf, "L %12ld %s\n", ts_now.tv_sec, s );
    */
}


void
lf_log_sd( const char* s, int d )
{
    char buf[64];
    sprintf(buf,"%d", d );
    lf_start('L');
    lf_add(s);
    lf_add(buf);
    lf_end();
    /*
    fprintf( lf, "L %12ld %s %d\n", ts_now.tv_sec, s, d );
    */
}




/* ************************************************************************** */

void
mqtt_subscribe(struct mosquitto *mqc) {
    int n;
    for ( n = 0; n < n_sub_str; ++n ) {
	printf(".. Subscribe \"%s\"\n", sub_str[n]);
	mosquitto_subscribe(mqc, NULL, sub_str[n], 0);
    }
}


void
connect_callback(struct mosquitto *mqc, void *obj, int result)
{
    lf_log_sd("connect: ", result);
    printf("Connected: %d\n", result);
    mqtt_subscribe(mqc);
}


void
disconnect_callback(struct mosquitto *mqc, void *obj, int result)
{
    lf_log_sd("disconnect: ", result);
    printf("Disonnected: %d\n", result);
}

#define PUT_BUF_MAX (2048)

char put_buf[PUT_BUF_MAX];


void
message_callback(struct mosquitto *mqc, void *obj,
		 const struct mosquitto_message *msg)
{
    const char*    topic = msg->topic;
    const char*    pload = msg->payload;
    struct timespec ts;

    int i;
    int t;

    i =  clock_gettime(CLOCK_REALTIME, &ts);
    if ( i ) {
	perror("clock_gettime: ");
	exit( EXIT_FAILURE );
    }

    t = match(topic);
    if ( t < 0 ) {
	static int bad_count = 25;
	if ( bad_count ) {
	    printf("Ignored topic(%d): %s\n",bad_count,topic);
	    --bad_count;
	}
	sprintf( put_buf,
		 "bad %.64s %ld %.64s\n",
		 topic,
		 ts.tv_sec,
		 pload
		 );
    }
    else if ( ttab[t].metric ) {
	sprintf(put_buf,
		"put %.64s %ld %.64s %.64s\n",
		ttab[t].metric,
		ts.tv_sec,
		pload,
		ttab[t].tagval);
    }
	
    lf_put( put_buf );

}


/* ************************************************************************** */

struct mosquitto* mqc = 0;		/* mosquitto client */

void
mq_init()
{
    int i;
    i = mosquitto_lib_init();
    if ( i != MOSQ_ERR_SUCCESS) {
	perror("mosquitto_lib_init: ");
	exit( EXIT_FAILURE );
    }
    
    mqc = mosquitto_new(mqtt_id, true, 0);
    if ( !mqc ) {
	perror("mosquitto_new: ");
	exit( EXIT_FAILURE );
    }

}


void
mq_fini()
{
    int i;

    if ( mqc ) {
	mosquitto_destroy(mqc);
	mqc = 0;
    }

    i = mosquitto_lib_cleanup();
    if ( i != MOSQ_ERR_SUCCESS) {
	perror("mosquitto_lib_cleanup: ");
	exit( EXIT_FAILURE );
    }

    
}



/* ************************************************************************** */

int
main(int argc, const char** argv)
{
    setbuf(stdout, 0);
    printf("Starting.\n");

    chdir("/var/local/mqtt2log");

    mqtt_id = strdup("mqtt2log");
    
    --argc;
    ++argv;
    if ( !argv || !*argv || !**argv ) {
	fprintf(stderr,"Usage: mqtt2log <config-file>\n");
	exit( EXIT_FAILURE );
    }
    else {
	int status = cfgf_read_file( *argv, tagtab );
	if ( status ) {
	    fprintf(stderr,"Errors in config file.\n");
	    exit( EXIT_FAILURE );
	}
    }

    
    
    lf_open();
    lf_log_s("Starting.");

    
    mq_init();

    mosquitto_connect_callback_set(mqc, connect_callback);
    mosquitto_disconnect_callback_set(mqc, disconnect_callback);
    mosquitto_message_callback_set(mqc, message_callback);

    do {
	int i;
	i = mosquitto_connect(mqc, mqtt_broker, mqtt_port, 60);
	if ( i != MOSQ_ERR_SUCCESS) {
	    perror("mosquitto_connect: ");
	    exit( EXIT_FAILURE );
	}

	do {
	    printf("Entering loop.\n");
	    i = mosquitto_loop_forever(mqc, 60000, 1);
	    while ( i != MOSQ_ERR_SUCCESS ) {
		lf_log_s("disconnected.");
		sleep(10);
		i = mosquitto_reconnect(mqc);
	    }
	} while(1);

	
    } while(1);


    mq_fini();
    
    lf_log_s("Stopping.");
    lf_close();
    return EXIT_SUCCESS;
}
