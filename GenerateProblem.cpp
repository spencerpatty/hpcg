
//@HEADER
// ************************************************************************
// 
//               HPCG: Simple Conjugate Gradient Benchmark Code
// Questions? Contact Michael A. Heroux (maherou@sandia.gov) 
// 
// ************************************************************************
//@HEADER

/////////////////////////////////////////////////////////////////////////

// Routine to read a sparse matrix, right hand side, initial guess, 
// and exact solution (as computed by a direct solver).

/////////////////////////////////////////////////////////////////////////

// nrow - number of rows of matrix (on this processor)

#ifdef DEBUG
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
#include <cstdlib>
#include <cstdio>
#include <cassert>
#endif

#include "GenerateProblem.hpp"

#ifdef USING_MPI
#include <mpi.h>
#endif

void GenerateProblem(const Geometry & geom, SparseMatrix & A, double **x, double **b, double **xexact) {
#ifdef DEBUG
	int debug = 1;
#else
	int debug = 0;
#endif

	int size = geom.size;
	int rank = geom.rank;
	int nx = geom.nx;
	int ny = geom.ny;
	int nz = geom.nz;
	int npx = geom.npx;
	int npy = geom.npy;
	int npz = geom.npz;
	int ipx = geom.ipx;
	int ipy = geom.ipy;
	int ipz = geom.ipz;
	int gnx = nx*npx;
	int gny = ny*npy;
	int gnz = nz*npz;

	int localNumberOfRows = nx*ny*nz; // This is the size of our subblock
	int numberOfNonzerosPerRow = 27; // We are approximating a 27-point finite element/volume/difference 3D stencil

	int totalNumberOfRows = localNumberOfRows*size; // Total number of grid points in mesh


	// Allocate arrays that are of length localNumberOfRows
	int * nonzerosInRow = new int[localNumberOfRows];
	int ** matrixIndices = new int*[localNumberOfRows];
	double ** matrixValues = new double*[localNumberOfRows];
	double ** matrixDiagonal = new double*[localNumberOfRows];

	*x = new double[localNumberOfRows];
	*b = new double[localNumberOfRows];
	*xexact = new double[localNumberOfRows];


	int localNumberOfNonzeros = 0;
	for (int iz=0; iz<nz; iz++) {
		int giz = ipz*nz+iz;
		for (int iy=0; iy<ny; iy++) {
			int giy = ipy*ny+iy;
			for (int ix=0; ix<nx; ix++) {
				int gix = ipx*nx+ix;
				int currentLocalRow = iz*nx*ny+iy*nx+ix;
				int currentGlobalRow = giz*gnx*gny+giy*gnx+gix;
				A.globalToLocalMap[currentGlobalRow] = currentLocalRow;
				int numberOfNonzerosInRow = 0;
				matrixValues[currentLocalRow] = new double[numberOfNonzerosPerRow]; // Allocate a row worth of values.
				matrixIndices[currentLocalRow] = new int[numberOfNonzerosPerRow]; // Allocate a row worth of indices.
				double * currentValuePointer = matrixValues[currentLocalRow]; // Pointer to current value in current row
				int * currentIndexPointer = matrixIndices[currentLocalRow]; // Pointer to current index in current row
				for (int sz=-1; sz<=1; sz++) {
					if (giz+sz>-1 && giz+sz<gnz) {
						for (int sy=-1; sy<=1; sy++) {
							if (giy+sy>-1 && giy+sy<gny) {
								for (int sx=-1; sx<=1; sx++) {
									if (gix+sx>-1 && gix+sx<gnx) {
										int curcol = currentGlobalRow+sz*gnx*gny+sy*gnx+sx;
										if (curcol==currentGlobalRow) {
											matrixDiagonal[currentLocalRow] = currentValuePointer;
											*currentValuePointer++ = 27.0;
										}
										else {
											*currentValuePointer++ = -1.0;
										}
										*currentIndexPointer++ = curcol;
										numberOfNonzerosInRow++;
									} // end x bounds test
								} // end sx loop
							} // end y bounds test
						} // end sy loop
					} // end z bounds test
				} // end sz loop
				nonzerosInRow[currentLocalRow] = numberOfNonzerosInRow;
				localNumberOfNonzeros += numberOfNonzerosInRow; // TODO: Protect this with an atomic or refactor
				(*x)[currentLocalRow] = 0.0;
				(*b)[currentLocalRow] = 27.0 - ((double) (numberOfNonzerosInRow-1));
				(*xexact)[currentLocalRow] = 1.0;
			} // end ix loop
		} // end iy loop
	} // end iz loop
#ifdef DEBUG
	cout 	  << "Process " << rank << " of " << size <<" has " << localNumberOfRows    << " rows."     << endl
			  << "Process " << rank << " of " << size <<" has " << localNumberOfNonzeros<< " nonzeros." <<endl;
#endif

#ifdef USING_MPI
  // Use MPI's reduce function to sum all nonzeros
  int totalNumberOfNonzeros = 0;
  MPI_Allreduce(&localNumberOfNonzeros, &totalNumberOfNonzeros, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
#else
  int totalNumberOfNonzeros = localNumberOfNonzeros;
#endif

	A.title = 0;
	A.totalNumberOfRows = totalNumberOfRows;
	A.totalNumberOfNonzeros = totalNumberOfNonzeros;
	A.localNumberOfRows = localNumberOfRows;
	A.localNumberOfColumns = localNumberOfRows;
	A.localNumberOfNonzeros = localNumberOfNonzeros;
	A.nonzerosInRow = nonzerosInRow;
	A.matrixIndices = matrixIndices;
	A.matrixValues = matrixValues;
	A.matrixDiagonal = matrixDiagonal;

	return;
}