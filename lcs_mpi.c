/*
	Trabalho 2 - MPI
	Vitoria Stavis de seq_araujo
	GRR20200243
	
	Compilar com: mpicc -O3 -lm -o mpi mpi_lcs.c  
	Rodar com: mpirun -np <n_procs> ./mpi

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "mpi.h"

// A utility function to find min of two integers 
int min(int a, int b) 
{ return (a < b)? a: b; } 
  
// A utility function to find min of three integers 
int min3(int a, int b, int c) 
{ return min(min(a, b), c);} 
  
// A utility function to find max of two integers 
int max(int a, int b) 
{ return (a > b)? a: b; } 

#define STD_TAG 0

typedef unsigned short mtype;

// le a sequencia, o parametro é o nome do arquivo
char* read_seq(char *fname)
{
	
	FILE *fseq = NULL;
	int size = 0;
	char *seq = NULL;

	int i = 0;
	
	fseq = fopen(fname, "rt");
	
	if (fseq == NULL )
	{
		printf("erro ao ler arquivo %s\n", fname);
		exit(1);
	}

	// encontrar tamanho da sequencia
	fseek(fseq, 0L, SEEK_END);
	size = ftell(fseq);
	rewind(fseq);

	// alocar memoria para sequencia
   
	seq = (char *) calloc(size + 1, sizeof(char));
	if (seq == NULL )
	{
		printf("erro ao alocar memoria %s.\n", fname);
		exit(1);
	}

	// le a sequencia e adiciona o \0 no final
	while (!feof(fseq))
	{
		seq[i] = fgetc(fseq);
		if ((seq[i] != '\n') && (seq[i] != EOF))
			i++;
	}	
	seq[i] = '\0';

	fclose(fseq);

	return seq;
}

// print matrix
void printMatrix(char * seqseq_a, char * seqseq_b, int ** scoreMatrix,  int sizeseq_a,
		 int sizeseq_b) {

	int i, j;

	//print header
	printf("Score Matrix:\n");
	printf("========================================\n");

	//print Lseq_cS score matrix al with sequences

	printf("    ");
	printf("%5c   ", ' ');

	for (j = 0; j < sizeseq_b; j++)
		printf("%5c   ", seqseq_b[j]);

	printf("\n");

	for (i = 0; i < sizeseq_a+1; i++)
	{
		if (i == 0)
			printf("    ");
		else
			printf("%c   ", seqseq_a[i - 1]);
		for (j = 0; j < sizeseq_b + 1; j++) {
			printf("%5d   ", scoreMatrix[i][j]);
		}
		printf("\n");
	}
	printf("========================================\n");
}

// retorna index de um caractere numa string, se nao tiver, retorna -1
int get_idx(char* str, int len, char c)
{
	int i;
	
    for(i = 0; i < len; i++)
    {		
        if(str[i] == c)
        {
            return i;
        }
    }

    return -1; 
}

int lcs_parallel(char* s1, char *s2, int len_s1, int len_s2, int rank, int size)
{
    int rows = len_s1 + 1;
    int cols = len_s2 + 1;

    int row, col;
    MPI_Status status;

    int dp[3][cols];

    for (int line = 1; line < rows+cols; line++)
	{
        int curr_line = line % 3;
        int prev_line = (line-1) % 3;
        int prev_prev_line = (line-2) % 3;

        int start_col =  max(0, line-rows); 
        int count = min3(line, (cols-start_col), rows); 

        int start, end;
        if (count <= size)
		{
            start = rank;
            end = min(rank, count-1);
        }
		else
		{
            float block_len = (float)count / size;
            start = round(block_len*rank);
            end = round(block_len*(rank+1))-1;
        }
  
        for (int j=start; j<=end; j++)
		{
            row = min(rows, line)-j-1;
            col = start_col+j;

            if (row==0 || col==0)
			{
                dp[curr_line][col] = 0;
            }
            else if (s1[row - 1] == s2[col - 1])
			{
                int upper_left = dp[prev_prev_line][col-1];
                dp[curr_line][col] = upper_left + 1;
            }
            else
			{
                int left = dp[prev_line][col-1];
                int up = dp[prev_line][col];
                dp[curr_line][col] = max(left, up);
            }

            if (j == start && rank > 0)
			{
                MPI_Send(&dp[curr_line][col], 1, MPI_INT, rank-1, 1, MPI_COMM_WORLD);
            }
            if (j == end && rank < size-1)
			{
                MPI_Send(&dp[curr_line][col], 1, MPI_INT, rank+1, 1, MPI_COMM_WORLD);
            }
        }

        int prev_index = start - 1;
        if (prev_index >= 0 && prev_index < count)
		{
            col = start_col+prev_index;
            MPI_Recv(&dp[curr_line][col], 1, MPI_INT, rank-1, 1, MPI_COMM_WORLD, &status);
        }

        int next_index = end + 1;
        if (next_index >= 0 && next_index < count)
		{
            col = start_col+next_index;
            MPI_Recv(&dp[curr_line][col], 1, MPI_INT, rank+1, 1, MPI_COMM_WORLD, &status);
        }
    }

    return dp[(rows+cols-1) % 3][cols-1];
}

void load_input(char **sa, int *sia, char **sb, int *sib, int rank)
{
	char *seq_a, *seq_b;
	int len_a, len_b;	

	if(rank == 0)
	{
	    // ler sequencias a e b
        seq_a = read_seq("fileA.in");
        seq_b = read_seq("fileB.in");
        len_a = strlen(seq_a);
        len_b = strlen(seq_b);
        
        MPI_Bcast(&len_a, 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(&len_b, 1, MPI_INT, 0, MPI_COMM_WORLD);       
    }
    else
    {
        MPI_Bcast(&len_a, 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(&len_b, 1, MPI_INT, 0, MPI_COMM_WORLD);
      
        seq_a = malloc(len_a+1);
        seq_b = malloc(len_b+1);       
    }

    MPI_Bcast(&seq_a[0], len_a, MPI_CHAR, 0, MPI_COMM_WORLD);
    MPI_Bcast(&seq_b[0], len_b, MPI_CHAR, 0, MPI_COMM_WORLD);
    
    *sa = seq_a;
    *sb = seq_b;
 	
    *sia = len_a;
    *sib = len_b;
}

int main(int argc, char ** argv)
{
	int rank;
    int num_procs;
   
    mtype res_par;						// variavel do score

	// inicializa MPI e pega rank do processo e n de processadores
    MPI_Init(&argc, &argv);	
    MPI_Comm_rank(MPI_COMM_WORLD, &rank); 		// rank do processo
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs); 	// numero total de processos

	// variaveis para medir o tempo
    double time_total, seq_time, bs, begin, end;	
	begin = MPI_Wtime();
    
	// adquire strings a b e c
    char *seq_a, *seq_b;
	int len_a, len_b;	
    load_input(&seq_a, &len_a, &seq_b, &len_b, rank);
	
	mtype** s_matrix;	

	// alocar e iniciar matriz S 	
	s_matrix = (mtype **)malloc((len_a + 1) * sizeof(mtype *));
	for(int k = 0; k < len_a + 1; k++)
	{		
		s_matrix[k] = (mtype *)calloc((len_b + 1), sizeof(mtype));
	}
	for(int k = 0 ;k < len_a + 1; k++)
	{
		for(int l = 0; l < len_b + 1; l++)
		{
			s_matrix[k][l]=0;
		}
	}

	if(rank == 0)
	{
		// conta o tempo sequencial
		bs = MPI_Wtime();
		seq_time = bs - begin;		
	}	
    
	// LCS paralelo
    res_par = lcs_parallel(seq_a, seq_b, len_a, len_b, rank, num_procs);
    	   
    if (rank == 0)
    {
		// conta o tempo total e printa os resultados
		end = MPI_Wtime();
		time_total = end - begin;	
        printf("res par: %d, tempo par: %f \n", res_par, time_total);
		// printf("%f\n", (seq_time*100/time_total)); // porcentagem de tempo sequencial
    }

	// libera memoria	
	for(int l=0;l<len_b+1;l++)
    {
            free(s_matrix[l]);
    }
	free(s_matrix);

	// Shutdown MPI 
    MPI_Finalize();
    return 0;
}
