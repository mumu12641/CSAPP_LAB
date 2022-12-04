/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    // 留给下次来做吧 想不出来
    // 参考万能的互联网
		// int i, j, m, n;
		// for (i = 0; i < N; i += 8){
		// 	for (j = 0; j < M; j += 8){
		// 		for (m = i; m < i + 8; ++m){
		// 			for (n = j; n < j + 8; ++n){
		// 				B[n][m] = A[m][n];
		// 			}
        //         }
        //     }
        // }
    if ((M == 32 && N == 32 )){
        int ii, jj, i, val1, val2, val3, val4, val5, val6, val7, val0;
        for(jj = 0; jj < M; jj += 8)
        {
            for(ii = 0; ii < N; ii += 8)
            {
                for(i = ii; i < ii + 8; i++)
                {
                    val0 = A[i][jj];
                    val1 = A[i][jj + 1];
                    val2 = A[i][jj + 2];
                    val3 = A[i][jj + 3];
                    val4 = A[i][jj + 4];
                    val5 = A[i][jj + 5];
                    val6 = A[i][jj + 6];
                    val7 = A[i][jj + 7];
                    B[jj][i] = val0;
                    B[jj + 1][i] = val1;
                    B[jj + 2][i] = val2;
                    B[jj + 3][i] = val3;
                    B[jj + 4][i] = val4;
                    B[jj + 5][i] = val5;
                    B[jj + 6][i] = val6;
                    B[jj + 7][i] = val7;
                }
            }
        }
    }
    if( M == 64 && N==64){
        int i, j, k,val0,val1,val2,val3;
		for (i = 0; i < N; i += 4){
			for (j = 0; j < M; j += 4){
                for(k = i; k < (i + 4);k++){
                    val0 = A[k][j];
					val1 = A[k][j+1];
					val2 = A[k][j+2];
					val3 = A[k][j+3];
					B[j][k] = val0;
					B[j+1][k] = val1;
					B[j+2][k] = val2;
					B[j+3][k] = val3;

                }
            }
        }
    }
    if(M == 61 && N == 67)
    {
        int ii, jj, i, j, val0, val1, val2, val3, val4, val5, val6, val7;
        for(ii = 0; ii + 16 < N; ii += 16)
        {
            for(jj = 0; jj + 16 < M; jj += 16)
            {
                for(i = ii; i < ii + 16; i++)
                {
                    val0 = A[i][jj + 0];
                    val1 = A[i][jj + 1];
                    val2 = A[i][jj + 2];
                    val3 = A[i][jj + 3];
                    val4 = A[i][jj + 4];
                    val5 = A[i][jj + 5];
                    val6 = A[i][jj + 6];
                    val7 = A[i][jj + 7];
                    B[jj + 0][i] = val0;
                    B[jj + 1][i] = val1;
                    B[jj + 2][i] = val2;
                    B[jj + 3][i] = val3;
                    B[jj + 4][i] = val4;
                    B[jj + 5][i] = val5;
                    B[jj + 6][i] = val6;
                    B[jj + 7][i] = val7;

                    val0 = A[i][jj + 8];
                    val1 = A[i][jj + 9];
                    val2 = A[i][jj + 10];
                    val3 = A[i][jj + 11];
                    val4 = A[i][jj + 12];
                    val5 = A[i][jj + 13];
                    val6 = A[i][jj + 14];
                    val7 = A[i][jj + 15];
                    B[jj + 8][i] = val0;
                    B[jj + 9][i] = val1;
                    B[jj + 10][i] = val2;
                    B[jj + 11][i] = val3;
                    B[jj + 12][i] = val4;
                    B[jj + 13][i] = val5;
                    B[jj + 14][i] = val6;
                    B[jj + 15][i] = val7;

                }
            }
        // 剩下的元素
        for(i = ii; i < N; i++)
            for(j = 0; j < M; j++)
                B[j][i] = A[i][j];
        for(i = 0; i < ii; i++)
            for(j = jj; j < M; j++)
                B[j][i] = A[i][j];
        }
    }
}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc); 

}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

