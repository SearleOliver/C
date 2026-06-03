
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/time.h>
#include <omp.h>


typedef struct color_pixel_struct {
    unsigned char r,g,b;
} color_pixel_type;

typedef struct color_image_struct
{
  int width, height;
  color_pixel_type * pixels;
} color_image_type;

typedef struct grey_image_struct
{
  int width, height;
  unsigned char * pixels;
} grey_image_type;


/**********************************************************************/

color_image_type * loadColorImage(char *filename){
  int i, width,height,max_value;
  char format[8];
  color_image_type * image;
  FILE * f = fopen(filename,"r");
  if (!f){
    fprintf(stderr,"Cannot open file %s...\n",filename);
    exit(-1);
  }
  fscanf(f,"%s\n",format);
  assert( (format[0]=='P' && format[1]=='3'));  // check P3 format
  while(fgetc(f)=='#') // commentaire
    {
      while(fgetc(f) != '\n'); // aller jusqu'a la fin de la ligne
    }
  fseek( f, -1, SEEK_CUR);
  fscanf(f,"%d %d\n", &width, &height);
  fscanf(f,"%d\n", &max_value);
  image = malloc(sizeof(color_image_type));
  assert(image != NULL);
  image->width = width;
  image->height = height;
  image->pixels = malloc(width*height*sizeof(color_pixel_type));
  assert(image->pixels != NULL);

  for(i=0 ; i<width*height ; i++){
      int r,g,b;
      fscanf(f,"%d %d %d", &r, &g, &b);
      image->pixels[i].r = (unsigned char) r;
      image->pixels[i].g = (unsigned char) g;
      image->pixels[i].b = (unsigned char) b;
    }
  fclose(f);
  return image;
}

/**********************************************************************/

grey_image_type * createGreyImage(int width, int height){
  grey_image_type * image = malloc(sizeof(grey_image_type));
  assert(image != NULL);
  image->width = width;
  image->height = height;
  image->pixels = malloc(width*height*sizeof(unsigned char));
  assert(image->pixels != NULL);
  return(image);
}

/**********************************************************************/

void saveGreyImage(char * filename, grey_image_type *image){
  int i;
  FILE * f = fopen(filename,"w");
  if (!f){
    fprintf(stderr,"Cannot open file %s...\n",filename);
    exit(-1);
  }
  fprintf(f,"P2\n%d %d\n255\n",image->width,image->height);
  for(i=0 ; i<image->width*image->height ; i++){
    fprintf(f,"%d\n",image->pixels[i]);
  }
  fclose(f);
}

/**********************************************************************/

void saveColorImage(char * filename, color_image_type *image){
  int i;
  FILE * f = fopen(filename,"w");
  if (!f){
    fprintf(stderr,"Cannot open file %s...\n",filename);
    exit(-1);
  }
  fprintf(f,"P3\n%d %d\n255\n",image->width,image->height);
  for(i=0 ; i<image->width*image->height ; i++){
    fprintf(f,"%d\n%d\n%d\n",image->pixels[i].r, image->pixels[i].g, image->pixels[i].b);
  }
  fclose(f);
}

/**********************************************************************/

void colorToGrey(color_image_type *col_img, grey_image_type *grey_img){
    for (int i=0; i < col_img->height ; i++)
      for (int j=0; j < col_img->width ; j++){
        int index = i * col_img->width + j;
        color_pixel_type *pix = &(col_img->pixels[index]);
        grey_img->pixels[index] = (299*pix->r + 587*pix->g + 114*pix->b)/1000;
      }
}

/**********************************************************************/

void colorToGreyPara(color_image_type *col_img, grey_image_type *grey_img, int threads){
  #pragma omp parallel for num_threads(threads)
  for (int i=0; i < col_img->height ; i++) {
    for (int j=0; j < col_img->width ; j++){
      int index = i * col_img->width + j;
      color_pixel_type *pix = &(col_img->pixels[index]);
      grey_img->pixels[index] = (299*pix->r + 587*pix->g + 114*pix->b)/1000;
    }
  }
}

/**********************************************************************/

void contrastAdjust (grey_image_type *grey_img){
    int H[256];
    int C[256];
    int S = grey_img->height * grey_img->width;
    for (int h=0; h<256;h++)
        H[h]=0;
    for (int i=0; i < grey_img->height ; i++)
      for (int j=0; j < grey_img->width ; j++)
        H[grey_img->pixels[i * grey_img->width + j]] +=1;
    C[0]=H[0];
    for (int h=1; h<256;h++)
        C[h]=C[h-1]+H[h];
    for (int i=0; i < grey_img->height ; i++)
      for (int j=0; j < grey_img->width ; j++){
        int index = i * grey_img->width + j;
        grey_img->pixels[index]= 255*C[grey_img->pixels[index]]/S;
    }
}

/**********************************************************************/

void contrastAdjustPara (grey_image_type *grey_img,int threads){
  int H[256];
  int C[256];
  int S = grey_img->height * grey_img->width;
  #pragma omp parallel num_threads(threads)
  {
  #pragma omp for
  for (int h=0; h<256;h++)
      H[h]=0;

  #pragma omp single
  {
  for (int i=0; i < grey_img->height ; i++)
    for (int j=0; j < grey_img->width ; j++)
      H[grey_img->pixels[i * grey_img->width + j]] +=1;

  C[0]=H[0];
    

  for (int h=1; h<256;h++)
    C[h]=C[h-1]+H[h];
  }
  
  
  #pragma omp for
  for (int i=0; i < grey_img->height ; i++){
    for (int j=0; j < grey_img->width ; j++){
      int index = i * grey_img->width + j;
      grey_img->pixels[index]= 255*C[grey_img->pixels[index]]/S;
    }
  }
  }
}

/**********************************************************************/

void toGreyContrastAdjustPara (color_image_type *col_img, grey_image_type *grey_img,int threads){
  int H[256];
  int C[256];
  int S = grey_img->height * grey_img->width;
  #pragma omp parallel num_threads(threads)
  {
  #pragma omp for
  for (int i=0; i < col_img->height ; i++) {
    for (int j=0; j < col_img->width ; j++){
      int index = i * col_img->width + j;
      color_pixel_type *pix = &(col_img->pixels[index]);
      grey_img->pixels[index] = (299*pix->r + 587*pix->g + 114*pix->b)/1000;
    }
  }
  #pragma omp for
  for (int h=0; h<256;h++)
      H[h]=0;

  #pragma omp single
  {
    for (int i=0; i < grey_img->height ; i++)
      for (int j=0; j < grey_img->width ; j++)
        H[grey_img->pixels[i * grey_img->width + j]] +=1;

    C[0]=H[0];
    

    for (int h=1; h<256;h++)
      C[h]=C[h-1]+H[h];
  }
  
  
  #pragma omp for
  for (int i=0; i < grey_img->height ; i++){
    for (int j=0; j < grey_img->width ; j++){
      int index = i * grey_img->width + j;
      grey_img->pixels[index]= 255*C[grey_img->pixels[index]]/S;
    }
  }
  }
}

/**********************************************************************/
int main(int argc, char ** argv){
  color_image_type * col_img;
  grey_image_type * grey_img;
  int i, j;
  double start, stop;

  if (argc != 3){
    printf("Usage: togrey <input image> <output image>\n");
    exit(-1);
  }
  char *input_file = argv[1];
  char *output_file = argv[2];

  col_img = loadColorImage(input_file);
  grey_img = createGreyImage(col_img->width, col_img->height);
  
  double t_seq[100], t_parasep2[100], t_parasep4[100], t_parasep8[100], t_parasep12[100], t_parasep16[100];
  double t_paracomb2[100], t_paracomb4[100], t_paracomb8[100], t_paracomb12[100], t_paracomb16[100];

  for (int i = 0; i < 100; i++) {
    start = omp_get_wtime(); colorToGrey(col_img, grey_img); contrastAdjust(grey_img); t_seq[i] = omp_get_wtime() - start;
    start = omp_get_wtime(); colorToGreyPara(col_img, grey_img,2); contrastAdjustPara(grey_img,2); t_parasep2[i] = omp_get_wtime() - start;
    start = omp_get_wtime(); colorToGreyPara(col_img, grey_img,4); contrastAdjustPara(grey_img,4); t_parasep4[i] = omp_get_wtime() - start;
    start = omp_get_wtime(); colorToGreyPara(col_img, grey_img,8); contrastAdjustPara(grey_img,8); t_parasep8[i] = omp_get_wtime() - start;
    start = omp_get_wtime(); colorToGreyPara(col_img, grey_img,12); contrastAdjustPara(grey_img,12); t_parasep12[i] = omp_get_wtime() - start;
    start = omp_get_wtime(); colorToGreyPara(col_img, grey_img,16); contrastAdjustPara(grey_img,16); t_parasep16[i] = omp_get_wtime() - start;
    start = omp_get_wtime(); toGreyContrastAdjustPara(col_img, grey_img, 2); t_paracomb2[i] = omp_get_wtime() - start;
    start = omp_get_wtime(); toGreyContrastAdjustPara(col_img, grey_img, 4); t_paracomb4[i] = omp_get_wtime() - start;
    start = omp_get_wtime(); toGreyContrastAdjustPara(col_img, grey_img, 8); t_paracomb8[i] = omp_get_wtime() - start;
    start = omp_get_wtime(); toGreyContrastAdjustPara(col_img, grey_img, 12); t_paracomb12[i] = omp_get_wtime() - start;
    start = omp_get_wtime(); toGreyContrastAdjustPara(col_img, grey_img, 16); t_paracomb16[i] = omp_get_wtime() - start;
  }
  FILE* f = fopen("resultats5.csv", "w");
  if (f == NULL) { fprintf(stderr, "Erreur ouverture fichier CSV\n"); return EXIT_FAILURE; }

  fprintf(f, "execution,sequentielle,paraSep2,paraSep4,paraSep8,paraSep12,paraSep16,paraComb2,paraComb4,paraComb8,paraComb12,paraComb16\n");
  for (i = 0; i < 100; i++)
    fprintf(f, "%d,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f\n", i+1, t_seq[i], t_parasep2[i], t_parasep4[i], t_parasep8[i], t_parasep12[i], t_parasep16[i], t_paracomb2[i],t_paracomb4[i], t_paracomb8[i],t_paracomb12[i], t_paracomb16[i]);

  fclose(f);
  printf("Résultats exportés dans resultats5.csv\n");
  
  saveGreyImage(output_file, grey_img);
  return EXIT_SUCCESS;
}
