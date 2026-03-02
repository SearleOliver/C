#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>

typedef unsigned char decalage ;

double freq_fr[26] = {
  0.076,0.009,0.032,0.036,0.147,0.010,0.009,0.007,
  0.075,0.006,0.001,0.055,0.027,0.071,0.053,0.030,
  0.014,0.065,0.079,0.070,0.063,0.018,0.001,0.004,
  0.003,0.001
};

#define CODE_ERREUR 255

decalage code ( char c ) {
  if ( isupper(c) ) return c - 'A' ;
  if ( islower(c) ) return c - 'a' ;
  /* erreur si ce n'est pas une lettre */
  return CODE_ERREUR ;
}

char minuscule ( decalage c ) {  return 'a' + ( c % 26 )  ; }

char majuscule ( decalage c ) { return 'A' + ( c % 26 )  ; }

decalage code_inverse ( decalage c ) { return ( 26 - c );  }

decalage chiffre_code ( decalage a , decalage b ) {return ( a+b ) % 26 ;}

char inverse_lettre ( void * __attribute__((unused))  cookie , char c ) {return minuscule ( code_inverse ( code ( c ) ) ) ;}


char * inverse_clef_old ( char * k ){
  int len = strlen ( k ) ;
  char * clef = malloc ( len + 1 ) ;
  for ( int i = 0 ; i < len ; i++ )
    clef[i] = inverse_lettre ( NULL , k[i] ) ;
  clef[len] = '\0' ;
  printf ( "clé %s inverse de %s\n" , clef , k ) ;
  return clef ;
}

char * strmap ( char * s , char ( *f ) ( void * , char ) , void * cookie ){
  int len = strlen ( s ) ;
  char * res = malloc ( ( len + 1 ) * sizeof ( char ) ) ;
  for ( int i = 0 ; i < len ; i++ )
    res[i] = f ( cookie , s[i] ) ;
  res[len] = '\0' ;
  return res ;
}

char * inverse_clef ( char * k ) { return strmap ( k , & inverse_lettre , NULL ) ; }

char chiffre_lettre ( char a , char b ) {
  if ( isupper ( a ) ) return majuscule ( chiffre_code(code(a),code(b)) ) ;
  if ( islower ( a ) ) return minuscule ( chiffre_code(code(a),code(b)) ) ;
  return '_' ;
}

struct key_cookie {
  int j ;
  char * key ;
} ;

char encode ( void * cookie_v , char c ){
  struct key_cookie * cookie = ( struct key_cookie * ) cookie_v ;
  if ( cookie->key[cookie->j] == 0 ) cookie->j = 0 ;
  if ( ! isalpha ( c ) ) return c ;
  return chiffre_lettre ( c , cookie->key[cookie->j++] ) ;
}

char * chiffre_chaine ( char * m , char * k ){
  struct key_cookie cookie = { .j = 0 , .key = k } ;
  return strmap ( m , & encode , & cookie ) ;
}

char * vigenere_chiffre ( char * m , char * k ){
  printf ( "chiffrement avec la clef \"%s\"\n" , k ) ;
  return chiffre_chaine ( m , k ) ;
}

char * vigenere_dechiffre ( char * m , char * k ){
  char * clef = inverse_clef ( k ) ;
  printf ( "Déchiffrement avec la clef \"%s\" inverse de \"%s\"\n" , clef , k ) ;
  char * res = chiffre_chaine ( m , clef ) ;
  free ( clef ) ;
  return res ;
}

char * cesar_chiffre ( char * m , char lettre ){
  char clef[2] ;
  clef[0] = lettre  ;
  clef[1] = '\0' ;
  return chiffre_chaine ( m , clef ) ;
}

char * cesar_dechiffre ( char * m ,  char lettre ){
  char clef[2] ;
  clef[0] = lettre ;
  clef[1] = '\0' ;
  char * clef_dechiffrement = inverse_clef ( clef ) ;
  char * res = chiffre_chaine ( m , clef_dechiffrement ) ;
  free ( clef_dechiffrement ) ;
  return res ;
}

int * calcule_frequences ( char * m ){
  int * occurrences = malloc ( 26 * sizeof ( int ) ) ;
  for ( int i = 0 ; i < 26 ; i++ )
    occurrences[i] = 0 ;

  int len = strlen ( m ) ;
  for ( int i = 0 ; i <  len ; i++ )
    if ( code(m[i]) != CODE_ERREUR )
      occurrences[code(m[i])]++ ;
  return occurrences ;
}

/* à compléter: faire des analyses fréquentielles ou utiliser la force
   brute pour déchiffrer les textes donnés dans l'énoncé */

int indice_max(int * tab){
  int max = 0;
  for (int i = 1; i < 26; i++)
    if (tab[i] > tab[max])
      max = i;
  return max;
}

// fonction qui fait un décalage vers E dans un fichier copie.
void cesar_E (char* texte){
  int copie= open("copie.txt",O_WRONLY|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR|S_IRGRP );
  if (copie == -1){
    perror("Erreur creation fichier copie.\n");
    exit(1);
  }
  // calcul frequences
  int * occurrences = calcule_frequences(texte);
  int max = indice_max(occurrences);
  int decalage = (max - code('E')+ 26) % 26;
  char lettre_clef = majuscule(decalage);

  // Affichage
  printf("Occurences dans le texte fourni :\n");
  for (int i = 0; i < 26; i++){
      printf("%c : %d\n", 'A'+i, occurrences[i]);
  }
  printf("Décalage de %d dans la copie du texte.\n",decalage);

  char * resultat = cesar_dechiffre(texte, lettre_clef);

  write(copie, resultat, strlen(resultat));

  close(copie);
  free(resultat);
  free(occurrences);
}

void test_cesar ( char * m ){
  printf("\n=== Test César ===\n");

  for (int i = 0; i < 26; i++) {
    char lettre = 'A' + i;
    char * res = cesar_dechiffre(m, lettre);
    printf("Clef %c :\n%s\n\n", lettre, res);
    free(res);
  }
}

void test_vigenere ( char * texte , int max ){
  printf("\n=== Test Vigenere ===\n");

  int len = strlen(texte);

  for (int n = 1; n <= max; n++) {

    char clef[n + 1];

    printf("\nLongueur clef testee : %d\n", n);

    for (int col = 0; col < n; col++) {

      char * sample = malloc(len + 1);
      int k = 0;

      int lettre_index = 0;

      for (int i = 0; i < len; i++) {
      if (!isalpha(texte[i]))
          continue;
      if (lettre_index % n == col)
          sample[k++] = texte[i];
      lettre_index++;
      }

      sample[k] = '\0';

      int * occ = calcule_frequences(sample);

      int max_i = indice_max(occ);

      /* On suppose que max correspond à 'E' */
      int decalage = (max_i - code('E') + 26) % 26;

      clef[col] = majuscule(decalage);

      free(occ);
      free(sample);
    }

    clef[n] = '\0';

    printf("Clef probable : %s\n", clef);

    char * res = vigenere_dechiffre(texte, clef);
    printf("Texte obtenu :\n%s\n", res);
    free(res);
  }
}

int main(int argc, char * argv[]){
  if (argc != 3){
    fprintf(stderr,"Erreur Usage : ./crypt fichier 1/2 (1 pour Cesar, 2 pour Vignere.)\n");
  }
  int source;

  source = open(argv[1], O_RDONLY);
  if (source == -1){
    perror("Erreur ouverture fichier source.\n");
    exit(2);
  }

  // Lecture du texte
  off_t taille = lseek(source, 0, SEEK_END);
  lseek(source, 0, SEEK_SET);

  char * texte = malloc(taille + 1);
  read(source, texte, taille);
  texte[taille] = '\0';

  if (strcmp(argv[2], "1") == 0){
    // Application de Cesar
    test_cesar(texte);
    cesar_E(texte);
  } else {
    // Application Vignere
    test_vigenere(texte, 4);
    // On trouve une clef "xatg"
    vigenere_dechiffre(texte, "xatg");
  }
  
  close(source);
  free(texte);
}


