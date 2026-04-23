#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>

/* fd (file descriptor) plus lisible que int */
typedef int fd ;
/* sortie prématurée */
void err ( int signal , char *msg ) {
  fprintf ( stderr , "%s\n" , msg ) ;
  exit ( signal ) ;
}
/* half-bytes */
unsigned char to_half_int ( unsigned char c ) {
  if ( ( '0' <= c ) && ( c <= '9' ) ) return c - '0' ;
  if ( ( 'A' <= c ) && ( c <= 'F' )) return 10+ (c - 'A') ;
  if ( ( 'a' <= c ) && ( c <= 'f' )) return 10+ (c - 'a') ;
  err(EXIT_FAILURE, "Not an hexadecimal stream") ;
  return 0 ; /* unreachable */
}
char convert[16] = {
  '0' , '1' , '2' , '3' ,
  '4' , '5' , '6' , '7' ,
  '8' , '9' , 'A' , 'B' ,
  'C' , 'D' , 'E' , 'F' } ;
/* bytes */
unsigned char to_int ( unsigned char * r )
{
  if ( ! isxdigit ( r[0] ) ) err(EXIT_FAILURE,"Error, letter in hexadecimal format is not an hexadecimal digit") ;
  if ( ! isxdigit ( r[1] ) ) err(EXIT_FAILURE,"Error, letter in hexadecimal format is not an hexadecimal digit") ;
  return to_half_int(r[0]) * 16 + to_half_int(r[1]) ;
}
void to_hexc ( unsigned char * res , unsigned char c )
{
#ifndef NDEBUG
  fprintf ( stderr , "read %c, %c / 16 = %d , %c %% 16 = %d\n" , c , c, c / 16 , c, c % 16 ) ;
#endif
  res[0] = convert[ c / 16 ] ;
  res[1] = convert[ c % 16 ] ;
}
/* files */
void to_bin( fd hex , fd binary ) {
  int nb_read ;
  unsigned char r[2] , w ;
  while (  ( nb_read = read ( hex , r , 2 ) )  == 2 )
    {
      w = to_int ( r ) ;
      if ( write ( binary , & w , 1 ) != 1 ) err ( EXIT_FAILURE ,"write error" ) ;
    }
  if ( nb_read == 0 ) return ;
  if ( ( nb_read == 1 ) && isspace (r[0]) ) to_bin(hex,binary) ;
  else err(EXIT_FAILURE,"Error, expected even number of characters") ;
}
void to_hex( fd binary , fd hex ) {
  unsigned char c , out[2] ;
  while ( read ( binary , & c , 1 ) == 1 )
    {
      to_hexc ( out , c ) ;
      if ( write ( hex , out , 2 ) != 2 ) err ( EXIT_FAILURE ,"write error" ) ;
    }
}

/* main */
int main ( int argc, char * argv[] )
{
  /* parametres */
  fd
    input = STDIN_FILENO ,
    output = STDOUT_FILENO ;
  void ( *map ) ( fd, fd ) = to_hex ;
  int i ;
  for ( i = 1 ; ( i < argc ) && ( argv[i][0] == '-' ) ; i++ )
      if ( strcmp ( argv[i] , "-r" ) == 0 ) map = to_bin ;
  if ( i < argc ) input = open ( argv[i++] , O_RDONLY ) ;
  if ( i < argc ) output = open ( argv[i++] , O_WRONLY | O_CREAT | O_TRUNC , 0644 ) ;
  map ( input , STDOUT_FILENO ) ;
  if ( input != STDIN_FILENO ) close ( input ) ;
  if ( output != STDOUT_FILENO ) close ( output ) ;
  return 0 ;
}
