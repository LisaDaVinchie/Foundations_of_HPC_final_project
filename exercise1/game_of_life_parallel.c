#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>

#include <time.h>
#include <omp.h>
#include <mpi.h>


// Definitions for the argument getting part
#define INIT 1
#define RUN  2

#define K_DFLT 100

#define ORDERED 0
#define STATIC  1


// char fname_deflt[] = "game_of_life.pgm";

int   action = INIT;
int   k      = K_DFLT;
int   e      = ORDERED;
int   n      = 10000;
int   s      = 1;
char *fname  = NULL;

#define MAXVAL 1


void write_pgm_image(char *image, int maxval, int xsize, int ysize, const char *image_name)
{
  FILE* image_file; 
  image_file = fopen(image_name, "wb"); 

  fprintf(image_file, "P4\n# generated by\n# put here your name\n%d %d\n%d\n", xsize, ysize, maxval);
  
  
  for (int i=0; i<xsize*ysize; i++)
            fprintf(image_file, "%c", image[i]);

  fclose(image_file); 
  return ;
}

void read_pgm_image( char **image, int *maxval, int *xsize, int *ysize, const char *image_name)

{
  FILE* image_file; 
  image_file = fopen(image_name, "r"); 

  *xsize = *ysize = *maxval = 0;
  
  char    MagicN[3];
  char   *line = NULL;
  size_t  k, n = 0;


  
  // get the Magic Number
  k = fscanf(image_file, "%2s%*c", MagicN);

  printf("Magic number read\n");
  // skip all the comments
  k = getline( &line, &n, image_file);
  printf("Comments skipped\n");
  while ( (k > 0) && (line[0]=='#') )
    k = getline( &line, &n, image_file);

  if (k > 0)
    {
      k = sscanf(line, "%d%*c%d%*c%d%*c", xsize, ysize, maxval);
      if ( k < 3 )
	fscanf(image_file, "%d%*c", maxval);
    }
  else
    {
      *maxval = -1;         // this is the signal that there was an I/O error
			    // while reading the image header
      free( line );
      return;
    }
  free( line );
  
  int color_depth = 1 + ( *maxval > 255 );
  unsigned int size = *xsize * *ysize * color_depth;
  
  if ( (*image = (char*)malloc( size )) == NULL )
    {
      fclose(image_file);
      *maxval = -2;         // this is the signal that memory was insufficient
      *xsize  = 0;
      *ysize  = 0;
      return;
    }
  
  if ( fread( *image, 1, size, image_file) != size )
    {
      free( image );
      image   = NULL;
      *maxval = -3;         // this is the signal that there was an i/o error
      *xsize  = 0;
      *ysize  = 0;
    }
    
    fclose(image_file);
  return;
}

void random_playground(char* image, int xsize, int ysize)
{
    int idx = 0;
    srand(time(NULL));
    for (int y = 0; y < ysize; y++){
        for (int x = 0; x < xsize; x++){
            image[idx] = (char)((int)rand()%2);
            idx++;
        }
    printf("\n");
    }
}


int count_live_neighbors(char* image, int row, int col, int xsize, int ysize){

  // Periodic boundary

  int xvalues[3];
  int yvalues[3];

  xvalues[1] = col;
  yvalues[1] = row;

  if(row == 0){
    yvalues[0] = ysize - 1;
    yvalues[2] = row + 1;
  }
  else if(row == ysize - 1){
    yvalues[0] = row - 1;
    yvalues[2] = 0;
  }
  else{
   yvalues[0] = row - 1;
   yvalues[2] = row + 1; 
  }

  if(col == 0){
    xvalues[0] = xsize - 1;
    xvalues[2] = col + 1;
  }
  else if(col == xsize - 1){
    xvalues[0] = col - 1;
    xvalues[2] = 0;
  }
  else{
   xvalues[0] = col - 1;
   xvalues[2] = col + 1; 
  }

  int sum = 0;
  for(int j = 0; j < 3; j++){
    for(int i = 0; i < 3; i++){
      if (i==1 && j==1){
        continue;
      }
      else{
        sum = sum + (int)(image[xvalues[i] + yvalues[j] * xsize]);
      }
    }
  }

return sum;
}


void ordered_evolution(char* image, int xsize, int ysize, int n, int s, char *destination_folder){

  int snap_idx = -1;
  if(s == 0){
    snap_idx = n;
  }
  else if(s > 0 && s < n){
    snap_idx = s;
  }
  else{
    printf("Wrong value for \"s\"\n");
  }
  
  for(int step = 0; step < n; step++){
    //ordered evolution
    for(int y = 0; y < ysize; y++){
      for (int x = 0; x < xsize; x++){
        //calculate status of cell in [x][y]
        int live_neighbors = -1;
        live_neighbors = count_live_neighbors(image, x, y, xsize, ysize);
        if (live_neighbors == 2 || live_neighbors == 3){
          image[x + y * xsize] = 1;
        }
        else if(live_neighbors < 0 || live_neighbors > 8){
          printf("There is an issue with the count of the neighbors that are alive, they cannot be %d\n", live_neighbors);
        }
        else{
          image[x + y * xsize] = 0;
        }
      }
    }

    if((step + 1)%snap_idx == 0){
      printf("\n");
      int idx = 0;
        for (int y = 0; y < ysize; y++){
            for (int x = 0; x < xsize; x++){
                idx++;
            }
      }
      char title[50];
      snprintf(title, 50, "%s_%d.pbm", destination_folder, step);
      write_pgm_image(image, 1, xsize, ysize, title);
    }
  }
}

void static_evolution(char* image, int xsize, int ysize, int n, int s, char *destination_folder){
  int snap_idx = -1;
  if(s == 0){
    snap_idx = n;
  }
  else if(s > 0 && s < n){
    snap_idx = s;
  }
  else{
    printf("Wrong value for \"s\"\n");
  }
  int temp_mat[xsize][ysize];
  for(int step = 0; step < n; step++){
      for(int y = 0; y < ysize; y++){
        for (int x = 0; x < xsize; x++){
          temp_mat[x][y] = -1;
          //calculate status of cell in [x][y]
          int live_neighbors = -1;
          live_neighbors = count_live_neighbors(image, x, y, xsize, ysize);
          if (live_neighbors == 2 || live_neighbors == 3){
            temp_mat[x][y] = 1;
          }
          else if(live_neighbors < 0 || live_neighbors > 8){
            printf("There is an issue with the count of the neighbors that are alive, they cannot be %d\n", live_neighbors);
          }
          else{
            temp_mat[x][y] = 0;
          }
        }
      }

    if((step + 1)%snap_idx == 0){
      int idx = 0;
      for(int y = 0; y < ysize; y++){
        for (int x = 0; x < xsize; x++){
          image[idx] = (char)temp_mat[x][y];
          idx++;
        }
      }
      char title[50];
      snprintf(title, 50, "%s_%d.pbm", destination_folder, step);
      write_pgm_image(image, 1, xsize, ysize, title);
    }
  }
}


int main ( int argc, char **argv )
{

//Get the arguments, copy of Tornatore's "get_args.c" program
  char *optstring = "irk:e:f:n:s:";

  printf("Get arguments\n");
  int c;
  while ((c = getopt(argc, argv, optstring)) != -1) {
    switch(c) {
      
    case 'i':
      action = INIT; break;
      
    case 'r':
      action = RUN; break;
      
    case 'k':
      k = atoi(optarg); break;

    case 'e':
      e = atoi(optarg); break;

    case 'f':
      fname = (char*)malloc(50);
      sprintf(fname, "%s", optarg );
      break;

    case 'n': //number of steps to be calculated
      n = atoi(optarg); break;

    case 's': //every how many steps a dump is saved
      s = atoi(optarg);break;

    default :
      printf("argument -%c not known\n", c ); break;
    }
  }
  printf("Arguments are: \naction = %d\nk = %d\ne = %d\nf = %s\nn_steps = %d\nn_dump = %d\n", action, k, e, fname, n, s);

  int xwidth = k;
  int ywidth = k;
  int maxval = MAXVAL;

  int rank, n_procs;
  int mpi_provided_thread_level;

  MPI_Init_thread( &argc, &argv, MPI_THREAD_FUNNELED, &mpi_provided_thread_level);
  // MPI_THREAD_FUNNELED: The process may be multi-threaded, but only the main
  // thread will make MPI calls (funneled through the main thread).

  // &mpi_provided_thread_level: This is an output parameter.
  // After calling MPI_Init_thread, this variable will hold the actual level
  // of thread support provided by the MPI library.


  printf("Provided Thread level: %d\n", mpi_provided_thread_level);

  if ( mpi_provided_thread_level < MPI_THREAD_FUNNELED ){
    printf("A problem arised when asking for MPI_THREAD_FUNNELED level\n");
    MPI_Finalize();
    exit( 1 );
  } 

  MPI_Status status;
  // Declare variable "status" of type "MPI_Status" to hold info about
  // the outcome of a communication


  // Get the rank of the process and the total number of processes 
  MPI_Comm_size(MPI_COMM_WORLD, &n_procs);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  printf("Rank = %d, size = %d\n", rank, n_procs);


  // Get the number of OpenMP threads
  int n_threads = 0;

  #pragma omp parallel
  {
    #pragma omp master
    {
      n_threads = omp_get_num_threads();
      printf("OpenMP threads: %d\n", n_threads);
    }
  }

  // Get the number of threads per process
  // int threads_per_procs = 0;
  #pragma omp parallel
  {
    int threads_per_procs = omp_get_num_threads();
    printf("Threads per process: %d\n", threads_per_procs); 
  }

  

  // Note that y = row index, x = col index

  // initialise playground
  if(action == INIT){

    // distribute rows between MPI processes

    int row_per_proc = xwidth / n_procs; // rows for each process
    // int remaining_rows = xwidth % n_procs; // remaining rows

    int startrow = row_per_proc * rank;
    int endrow = row_per_proc * (rank + 1);
    // if (rank < remaining_rows){
    //   row_per_proc++;
    // }

    int n_cells = row_per_proc * xwidth;


    char* temp_image = NULL;
    temp_image = (char*)malloc(n_cells * sizeof(char));

    if(temp_image == NULL){
      printf("Memory allocation of \"image\" failed\n");
      free(fname);
      return 1;
    }

    #pragma omp parallel
    {
      int thread_id = omp_get_num_threads();
      
      // int idx = 0;
      srand(time(NULL));
      
      // printf("\nInitial playground, %d part\n", rank);
      int idx = 0;
      // random_playground(image, row_per_proc, ywidth);
      #pragma omp for schedule(static, row_per_proc)
        for (int y = startrow; y < endrow; y++){
            for (int x = 0; x < xwidth; x++){
                temp_image[x + y * xwidth] = (char)((int)rand()%2);
                // printf("%d ", (int)image[idx]);
                idx++;
            }
            printf("\n");
          }
    }

    char* image = NULL;

    image = (char*)malloc(xwidth * ywidth * sizeof(char));

    MPI_Gather(image, row_per_proc * xwidth, MPI_CHAR, image, row_per_proc * xwidth, MPI_CHAR, 0, MPI_COMM_WORLD);

    if(rank == 0){
      int idx = 0;
      for (int x = 0; x < ywidth; x++){
        for(int y = 0; y < xwidth; y++){
          printf("%d ", (int)image[idx]);
          idx++;
        }
        printf("\n");
      }
    }

    // MPI_File filename;




    //write_pgm_image(image, maxval, xwidth, ywidth, fname);

  }

  else if(action == RUN){
    char* image = NULL;
    // int maxval, xsize, ysize;
    // image = (char*)malloc(xwidth * ywidth * sizeof(char));
    printf("About to read playground\n");
    // char *source_image = "snapshots_static/initial_conditions.pgm";
    read_pgm_image(&image, &maxval, &xwidth, &ywidth, fname);
    printf("Playground read\n");

    int idx = 0;
    for (int y = 0; y < ywidth; y++){
      for (int x = 0; x < xwidth; x++){
          idx++;
      }
    } 
    if(e == ORDERED){
      ordered_evolution(image, xwidth, ywidth, n, s, "snapshots_ordered/snap");
    }
    else if(e == STATIC){
      static_evolution(image, xwidth, ywidth, n, s, "snapshots_static/snap");
    }
    else{
      printf("Invalid value for flag \"-e\"\n");
      free(fname);
      return 1;
    }
   free(image); 
  }

  else{
    printf("Invalid value for variable \"action\"\n");
    free(fname);
    return 1;
  }
  MPI_Finalize();

  free(fname);
  return 0;
}
