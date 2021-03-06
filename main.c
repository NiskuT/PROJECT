
#include <stdio.h>      /**< printf, fgets */
#include <stdlib.h>     /**< exit */
#include <unistd.h>     /**< close */
#include <sys/stat.h>   /**< open */
#include <fcntl.h>      /**< open */
#include <signal.h>     /**< sigaction */
#include <stdbool.h>    /**< bool */
#include <termios.h>    /**< tcflush */
 
/** Configuration */
 
#define ActiveTcflushTest 1
const char uartPath[] = "/dev/ttyAMA0";
 
/** Prototype */
 
int main( int argc, char * argv[] );
int transmitter();
int receptor();
 
/** Partie commune */
 
bool run;
 
/**< Fonction sigint en cas de CTRL+C */
void SIGINT_HANDLE( int signal )
{
  printf( "Fermeture par SIGINT : %i\n", signal );
  run = false;
}
 
/**< Permet d'affecter un signal à une fonction */
int add_signal ( int sig, void ( *h )( int ), int options )
{
  int r;
  struct sigaction s;
  s.sa_handler = h;
  sigemptyset ( &s.sa_mask );
  s.sa_flags = options;
  r = sigaction ( sig, &s, NULL );
  if ( r < 0 ) perror ( __func__ );
  return r;
}
 
void help()
{
  printf( "Usage : ./uart_test <mode>\n" );
  printf( "  <mode> : R|r pour lire\n" );
  printf( "  <mode> : W|w pour écrire\n" );
  printf( "\n" );
}
 
int main( int argc, char * argv[] )
{
  if( argc <= 1 ) { help(); exit( EXIT_FAILURE ); }
  //
  if( ( argv[1][0] == 'W' ) | ( argv[1][0] == 'w' ) )
    return transmitter();
  else if( ( argv[1][0] == 'R' ) | ( argv[1][0] == 'r' ) )
    return receptor();
  //
  help();
  exit( EXIT_FAILURE );
}
 
 
/** Partie Emetteur */
int transmitter()
{
  bool error = false;
  run = true;
 
  printf( "Ouverture en écriture : open( uartPath, O_WRONLY )...\n" );
 
  int uartWrite = open( uartPath, O_WRONLY );
  if( uartWrite < 0 ) {  perror( "open uart to write" ); return EXIT_FAILURE;}
 
  /**< Touhes CTRL-C */
  add_signal( SIGINT, SIGINT_HANDLE, 0 );
 
  /**< La taille du Buffer est arbitraire,
   * cela prendra juste plusieurs tours au buffer de STDIN pour se remplir
   * puis se vider s'il est surchargé. */
  char requestBuffer[64];
 
  ssize_t requestLength, sendLength, alreadySend;
  printf( "Saisir du texte à envoyer : \n" ); 
  while( run && !error )
  {
    /** ! Lecture ! */
 
    /**< Lecture stdin (fd=0) */
    requestLength = read( 0, requestBuffer, sizeof( requestBuffer ) );
 
    /**< Erreur de lecture */
    if( requestLength < 0 )  {  perror( "read stdin" ); error = true; break; }
     /**< Touhes CTRL-D */
    if( requestLength == 0 )  break;
 
    /** ! Envoi ! */
    alreadySend = 0;
    do
    {
      /**< Decalage auto avec "alreadySend", si le message n'est pas envoyé entièrement du premier coup */
      sendLength = write( uartWrite, requestBuffer + alreadySend, requestLength - alreadySend );
 
      /**< Erreur d'ecriture */
      if( sendLength < 0 )  {  perror( "write uart" ); error = true; run = false; break; }
 
      /**< Incrementation pour le decalage auto (si necessaire) */
      alreadySend += sendLength;
    }
    while ( alreadySend < requestLength );
 
    /**< Doit flush le buffer (système) de l'uart (cf. http://stackoverflow.com/a/13013771 ) */
    sleep(2); // a essayer au cas ou
    int tcFlushOK = tcflush( uartWrite, TCIOFLUSH );
    #if ActiveTcflushTest == 1
    if( tcFlushOK < 0 ) {  perror( "tcflush uartWrite" ); error = true; break; }
    #endif
  }
  close( uartWrite );
  return error ? EXIT_FAILURE : EXIT_SUCCESS;
}
 
 
/** Partie Recepteur */
int receptor()
{
  bool error = false;
  run = true;
 
  printf( "Ouverture en lecture : open( uartPath, O_RDONLY )...\n" );
 
  int uartRead = open( uartPath, O_RDONLY );
  if( uartRead < 0 ) {  perror( "open uart to read" ); return EXIT_FAILURE;}
 
  /**< Touhes CTRL-C */
  add_signal( SIGINT, SIGINT_HANDLE, 0 );
 
  /**< La taille du Buffer est arbitraire,
   * cela prendra juste plusieurs tours au buffer de l'uart pour se vider s'il est surchargé. */
  char responseBuffer[64];
 
  ssize_t responseLength;
  printf( "Attente du texte...\n" );
 
 
  while( run && !error )
  {
    /** ! Lecture ! */
 
    /**< Lecture uart (fd=???) */
    responseLength = read( uartRead, responseBuffer, sizeof( responseBuffer ) );
 
    /**< Erreur de lecture */
    if( responseLength < 0 )  {  perror( "read uart" ); error = true; break; }
 
    /**< Touhes CTRL-D provenant de l'émetteur (fin de transmission) */
    if( responseLength == 0 )  break;
 
    /** ! Affichage ! */
 
    /**< Affichage des données sur stdout (fd=1). Le cas de problème sur stdout est quasi inexistant */
    write( 1, responseBuffer, responseLength );
 
 
    /**< Doit flush le buffer (système) de l'uart (cf. http://stackoverflow.com/a/13013771 ) */
    sleep(2); // a essayer au cas ou
    int tcFlushOK = tcflush( uartRead, TCIOFLUSH );
    #if ActiveTcflushTest == 1
    if( tcFlushOK < 0 ) {  perror( "tcflush uartRead" ); error = true; break; }
    #endif
  }
 
  close( uartRead );
  return error ? EXIT_FAILURE : EXIT_SUCCESS;
}
