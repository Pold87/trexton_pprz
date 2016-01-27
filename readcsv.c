#include <stdio.h>
#include <string.h>
#include <stdlib.h>


int readcsv_test(int argc, char *argv[])
{
  char *filename = "foo.csv";
  int num_textons = 33;
  int size_texton = 5;

  double *textons[num_textons];

  read_textons_from_csv(textons, filename, num_textons, size_texton);
  return 0;
}

int read_textons_from_csv(double *textons[], char *filename, int num_textons, int size_texton) {
  
  int total_patch_size = size_texton * size_texton;

  char buf[1024];
  FILE *fp = fopen(filename, "r");
 
  if (!fp) {
    fprintf(stderr, "Can't open file.\n");
    return -1;
  }
 
   char *record, *line;
   int i = 0, j = 0;

   double result;

   while((line = fgets(buf, sizeof(buf), fp)) != NULL)
   {
     record = strtok(line, ",");
     textons[i] = malloc(num_textons * sizeof(double));
     for (j = 0; j < total_patch_size; j++) 
     {
       result = atof(record); /* Convert string to double */
       textons[i][j] = result; 
       record = strtok(NULL, ","); /* Read next string from line*/
     }
     ++i ;
   }
   fclose(fp);
   
   int print_vals = 0;
   
   if (print_vals) {
   int k, l;
   for (k = 0; k < num_textons; k++) {
     for (l = 0; l < total_patch_size; l++) {
       printf("%f ", textons[k][l]);
     }
     printf("\n");
   }
   }

   return 0;
   
}
