# include <stdlib.h>
# include <stdio.h>
#include <omp.h>
#include <sys/time.h>

#define INFINITE (1<<30) // a very large positive integer

struct direct_edge_struct;
struct direct_edge_struct {
  int destination_node;
  int weight;
  struct direct_edge_struct *next;
};

typedef struct {
    int value;
    char pad[60];
} __attribute__((aligned(64))) padded_int;


int num_nodes, num_edges;
// table 'edges' is used to store all edge data
//   (instead of dynamically allocating memory at each edge creation)
struct direct_edge_struct *edges;
// edge_counter is used to allocate entries in table 'edges'
int edge_counter = 0;
// table 'nodes' contains the direct edges out of each node
//  'node[i]' is a linked list to all edges starting from node i
struct direct_edge_struct **nodes;
int* d;
char* P;

int main ( int argc, char **argv );
void read_graph(char *filename);
void dijkstra();
void dijkstra2(int threads);
void dijkstra3(int threads);
void dijkstra4(int threads);
void dijkstraPara(int num_threads);

/******************************************************************************/
int main ( int argc, char **argv ){

  if (argc < 2){
    fprintf(stderr,"Usage: dijkstra <graph file name>\n");
    exit(-1);
  }
  else
    read_graph(argv[1]);

  posix_memalign((void**)&d, 64, num_nodes * sizeof(int));
  posix_memalign((void**)&P, 64, num_nodes * sizeof(char));

  
  double start, stop;
  double t_seq[100],t_para12[100],t_para8[100],t_para4[100];

  for(int i =0; i<5;i++){
    start = omp_get_wtime(); dijkstra3(12); t_para12[i] = omp_get_wtime() - start;
    start = omp_get_wtime(); dijkstra3(8); t_para8[i] = omp_get_wtime() - start;
    start = omp_get_wtime(); dijkstra3(4); t_para4[i] = omp_get_wtime() - start;
    //start = omp_get_wtime(); dijkstra(); t_seq[i] = omp_get_wtime() - start;
    t_seq[i]=0;
    //t_para4[i]=0;
    //t_para8[i]=0;
    //t_para12[i]=0;
    printf("boucle %d done.\n",i+1);
  }


  FILE* f = fopen("resultats6ny.csv", "w");
  if (f == NULL) { fprintf(stderr, "Erreur ouverture fichier CSV\n"); return EXIT_FAILURE; }

  fprintf(f, "execution,seq,para4,para8,para12\n");
  for (int i = 0; i < 5; i++)
    fprintf(f, "%d,%.6f,%.6f,%.6f,%.6f\n", i+1, t_seq[i],t_para4[i],t_para8[i],t_para12[i]);

  fclose(f);
  printf("Résultats exportés dans resultats6ny.csv\n");


  free(nodes);
  free(edges);
  free(d);
  free(P);
  return 0;
}

/******************************************************************************/
static inline int get_distance(int node1, int node2){
  // return distance between node1 and node2
  //   0 if node1==node2
  //   weight of edge if any between node1 and node2
  //   INFINITE otherwise
  if (node1 == node2)
    return 0;
  struct direct_edge_struct *edge = nodes[node1];
  while (edge != NULL){
    if (edge->destination_node == node2)
      return edge->weight;
    edge = edge->next;
  }
  // node2 has not been found as a direct neighbour of node 1
  return INFINITE;
}

/******************************************************************************/
void dijkstra(){
  // returns computation time

  int shortest_dist;
  int nearest_node;

  P[0] = 1;
  for (int i = 1; i < num_nodes; i++)
    P[i] = 0;

  for (int i = 0; i < num_nodes; i++)
    d[i] = get_distance(0,i);

  for (int step = 1; step < num_nodes; step++ ){
    // find the nearest node
    shortest_dist = INFINITE;
    nearest_node = -1;
    for (int i = 0; i < num_nodes; i++){
        if ( !P[i] && d[i] < shortest_dist ){
        shortest_dist = d[i];
        nearest_node = i;
      }
    }

    if ( nearest_node == - 1 ){
      fprintf(stderr,"Warning: Search ended early, the graph might not be connected.\n" );
      break;
    }

    P[nearest_node] = 1;
    for (int i = 0; i < num_nodes; i++)
      if ( !P[i] ){
        int dist = get_distance(nearest_node,i);
        if ( dist < INFINITE )
          if ( d[nearest_node] + dist < d[i] )
            d[i] = d[nearest_node] + dist;
      }
  }
}

void dijkstra2(int threads)
{
  int shortest_dist;
  int nearest_node;

  padded_int *local_dist = malloc(threads * sizeof(padded_int));
  padded_int *local_node = malloc(threads * sizeof(padded_int));

  P[0] = 1;

  #pragma omp parallel num_threads(threads)
  {
    int me = omp_get_thread_num();
    int my_dist;
    int my_node;

    #pragma omp for schedule(static) nowait
    for (int i = 1; i < num_nodes; i++)
      P[i] = 0;

    #pragma omp for schedule(static)
    for (int i = 0; i < num_nodes; i++)
      d[i] = get_distance(0, i);

    for (int step = 1; step < num_nodes; step++){
      my_dist = INFINITE;
      my_node = -1;

      #pragma omp for schedule(static)
      for (int i = 0; i < num_nodes; i++){
        if (!P[i]){
          int val = d[i];
          if (val < my_dist){
            my_dist = val;
            my_node = i;
          }
        }
      }

      local_dist[me].value = my_dist;
      local_node[me].value = my_node;

      #pragma omp barrier

      #pragma omp single
      {
        shortest_dist = INFINITE;
        nearest_node = -1;

        for (int t = 0; t < threads; t++){
          if (local_node[t].value != -1 && local_dist[t].value < shortest_dist){
            shortest_dist = local_dist[t].value;
            nearest_node = local_node[t].value;
          }
        }
      }

      if (nearest_node == -1){
        #pragma omp single
        fprintf(stderr, "Warning: Search ended early, graph might not be connected.\n");
        break;
      }

      #pragma omp single
      P[nearest_node] = 1;

      #pragma omp for schedule(static,1024)
      for (int i = 0; i < num_nodes; i++){
        if (!P[i]){
          int dist = get_distance(nearest_node, i);

          if (dist < INFINITE){
            int new_dist = shortest_dist + dist;
            if (new_dist < d[i])
              d[i] = new_dist;
          }
        }
      }
    }
  }

  free(local_dist);
  free(local_node);
}


void dijkstra3(int threads){
  int shortest_dist;
  int nearest_node;
  padded_int *local_dist = malloc(threads * sizeof(padded_int));
  padded_int *local_node = malloc(threads * sizeof(padded_int));
  P[0] = 1;

  #pragma omp parallel num_threads(threads)
  {
  int me = omp_get_thread_num();

  #pragma omp for schedule(static) nowait
  for (int i = 1; i < num_nodes; i++)
    P[i] = 0;

  #pragma omp for schedule(static)
  for (int i = 0; i < num_nodes; i++)
    d[i] = get_distance(0, i);

  for (int step = 1; step < num_nodes; step++){
    local_dist[me].value = INFINITE;
    local_node[me].value = -1;
    #pragma omp for schedule(static)
    for (int i = 0; i < num_nodes; i++){
      if (!P[i]){
        int val = d[i];
        if (val < local_dist[me].value){
          local_dist[me].value = val;
          local_node[me].value = i;
        }
      }
    }
    #pragma omp single
    {
    shortest_dist = INFINITE;
    nearest_node = -1;
    for (int t = 0; t < threads; t++){
      if (local_node[t].value != -1 && local_dist[t].value < shortest_dist){
        shortest_dist = local_dist[t].value;
        nearest_node = local_node[t].value;
        }
      }
    }

    if (nearest_node == -1){
      #pragma omp single
      fprintf(stderr, "Warning: Search ended early, graph might not be connected.\n");
      break;
    }
    #pragma omp single
    P[nearest_node] = 1;

    #pragma omp for schedule(static)
    for (int i = 0; i < num_nodes; i++){
      if (!P[i]){
      int dist = get_distance(nearest_node, i);
      if (dist < INFINITE){
        int new_dist = shortest_dist + dist;
        if (new_dist < d[i])
          d[i] = new_dist;
        }
      }
    }
    }
    }
  free(local_dist);
  free(local_node);
}

void dijkstra4(int threads){
  int shortest_dist;
  int nearest_node;
  padded_int *local_dist = malloc(threads * sizeof(padded_int));
  padded_int *local_node = malloc(threads * sizeof(padded_int));
  P[0] = 1;

  #pragma omp parallel num_threads(threads)
  {
  int me = omp_get_thread_num();

  #pragma omp for schedule(static) nowait
  for (int i = 1; i < num_nodes; i++)
    P[i] = 0;

  #pragma omp for schedule(static)
  for (int i = 0; i < num_nodes; i++)
    d[i] = get_distance(0, i);

  for (int step = 1; step < num_nodes; step++){
    local_dist[me].value = INFINITE;
    local_node[me].value = -1;
    #pragma omp for schedule(static)
    for (int i = 0; i < num_nodes; i++){
      if (!P[i]){
        int val = d[i];
        if (val < local_dist[me].value){
          local_dist[me].value = val;
          local_node[me].value = i;
        }
      }
    }
    #pragma omp single
    {
    shortest_dist = INFINITE;
    nearest_node = -1;
    for (int t = 0; t < threads; t++){
      if (local_node[t].value != -1 && local_dist[t].value < shortest_dist){
        shortest_dist = local_dist[t].value;
        nearest_node = local_node[t].value;
        }
      }
    }

    if (nearest_node == -1){
      #pragma omp single
      fprintf(stderr, "Warning: Search ended early, graph might not be connected.\n");
      break;
    }

    #pragma omp single
    {
    P[nearest_node] = 1;

    struct direct_edge_struct *edge = nodes[nearest_node];

    while (edge != NULL)
    {
      int i = edge->destination_node;
      int dist = edge->weight;

      if (!P[i])
      {
        if (d[nearest_node] + dist < d[i])
          d[i] = d[nearest_node] + dist;
      }

      edge = edge->next;
    }
  }
  }
  }
}

/******************************************************************************/
void read_graph(char *filename){
  char line[256];
  int node1, node2, weight;

  FILE *graph = fopen(filename,"r");
  if (graph == NULL){
    fprintf(stderr,"File %s not found.\n",filename);
    exit(-1);
  }

  while (fgets(line, 256, graph) != NULL){
    switch(line[0]){
      case 'c': // comment
        break;

      case 'p': // graph size
        if (sscanf(&(line[5]),"%d %d\n", &num_nodes, &num_edges) != 2){
          fprintf(stderr,"Error in file format in line:\n");
          fprintf(stderr, "%s", line);
          exit(-1);
        }
        else
          fprintf(stderr,"Graph contains %d nodes and %d edges\n", num_nodes, num_edges);
          edges = malloc(num_edges*2 * sizeof(struct direct_edge_struct));
          if (edges == NULL){
            fprintf(stderr,"Error: cannot allocate memory.\n");
            exit(-1);
          }
          nodes = malloc(num_nodes * sizeof(struct direct_edge_struct *));
          if (nodes == NULL){
            fprintf(stderr,"Error: cannot allocate memory.\n");
            exit(-1);
          }
          for (int i=0; i<num_nodes; i++)
            nodes[i] = NULL;

          d = malloc(num_nodes * sizeof(int));
          if (d == NULL){
            fprintf(stderr,"Error: cannot allocate memory.\n");
            exit(-1);
          }
          P = malloc(num_nodes * sizeof(char));
          if (P == NULL){
            fprintf(stderr,"Error: cannot allocate memory.\n");
            exit(-1);
          }
        break;

      case 'a': // edge definition
        if (sscanf(&(line[2]),"%d %d %d\n", &node1, &node2, &weight) != 3){
          fprintf(stderr,"Error in file format in line:\n");
          fprintf(stderr, "%s", line);
          exit(-1);
        }
        node1--; node2--; // number nodes from 0
        //distance[node1-1][node2-1] = weight;
        struct direct_edge_struct *new_edge;
        struct direct_edge_struct *e;
        new_edge = &edges[edge_counter++];
        new_edge->destination_node = node2;
        new_edge->weight = weight;
        new_edge->next = NULL;
        if (nodes[node1] == NULL)
          nodes[node1] = new_edge;
        else {
          e = nodes[node1];
          while (e->next != NULL)
            e = e->next;
          e->next = new_edge;
        }
        new_edge = &edges[edge_counter++];
        new_edge->destination_node = node1;
        new_edge->weight = weight;
        new_edge->next = NULL;
        if (nodes[node2] == NULL)
          nodes[node2] = new_edge;
        else {
          e = nodes[node2];
          while (e->next != NULL)
            e = e->next;
          e->next = new_edge;
        }
        break;
    }
  }
  fclose(graph);
}


void dijkstraPara(int num_threads)
{
    P[0] = 1;

    int global_min_dist;
    int global_min_node;

    #pragma omp parallel num_threads(num_threads) \
        shared(global_min_dist, global_min_node)
    {
        #pragma omp for schedule(static)
        for (int i = 1; i < num_nodes; i++)
            P[i] = 0;

        #pragma omp for schedule(static)
        for (int i = 0; i < num_nodes; i++)
            d[i] = get_distance(0, i);

        for (int step = 1; step < num_nodes; step++)
        {
            int local_min_dist = INFINITE;
            int local_min_node = -1;

            #pragma omp single
            {
                global_min_dist = INFINITE;
                global_min_node = -1;
            }

            /* recherche du minimum local */
            #pragma omp for schedule(static)
            for (int i = 0; i < num_nodes; i++)
            {
                if (!P[i] && d[i] < local_min_dist)
                {
                    local_min_dist = d[i];
                    local_min_node = i;
                }
            }

            /* réduction manuelle */
            if (local_min_node != -1)
            {
                #pragma omp critical
                {
                    if (local_min_dist < global_min_dist)
                    {
                        global_min_dist = local_min_dist;
                        global_min_node = local_min_node;
                    }
                }
            }

            /* attendre que tous les threads aient participé */
            #pragma omp barrier

            #pragma omp single
            {
                if (global_min_node != -1)
                    P[global_min_node] = 1;
            }

            #pragma omp barrier

            if (global_min_node == -1)
            {
                #pragma omp single
                {
                    fprintf(stderr,
                            "Warning: Search ended early, "
                            "the graph might not be connected.\n");
                }

                break;
            }

            int base_dist = d[global_min_node];

            /* relaxation */
            #pragma omp for schedule(static)
            for (int i = 0; i < num_nodes; i++)
            {
                if (!P[i])
                {
                    int dist = get_distance(global_min_node, i);

                    if (dist < INFINITE)
                    {
                        int new_dist = base_dist + dist;

                        if (new_dist < d[i])
                            d[i] = new_dist;
                    }
                }
            }

            #pragma omp barrier
        }
    }
}