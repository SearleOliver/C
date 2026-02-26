#include <stdio.h>

int main(void)
{
    int n,n2;
    printf("DONNE TON ENTIER : ");
    fflush(stdout); //VIDAGE DU TAMPON DEQ EESGLES
    scanf("%d",&n);
    printf("DONNE UN AUTRE : ");
    fflush(stdout); //2EME VIDAGE DU TAMPON DEQ EESGLES
    scanf("%d",&n2);
    int s = n + n2;
    int p = n*n2;
    printf("la somme des nombres est %d, leur produit est %d\n\a",s,p);
    return 0;

}