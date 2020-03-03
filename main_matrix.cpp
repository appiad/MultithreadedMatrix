#include <string>
#include <vector>
#include <iostream>
#include <chrono>
#include <cassert>
#include <utility>
#include "Matrix.h"
#include <time.h>

void get_start_time(std::chrono::high_resolution_clock::time_point& start){
	start= std::chrono::high_resolution_clock::now();
}

void get_finish_time(std::chrono::high_resolution_clock::time_point& finish){
	finish= std::chrono::high_resolution_clock::now();
}

//Runs a single threaded and multithreaded transpose operation on a 
//Matrix with the given dimensions. 
void test_transpose(int num_cols, int num_rows){
	std::chrono::high_resolution_clock::time_point start;
	std::chrono::high_resolution_clock::time_point finish;
	std::chrono::duration<double> elapsed;

	std::cout<< "----------------------------------" << std::endl;
	std::cout << "Transposing " << num_rows << " x " <<num_cols << " Matrix" << std::endl;
 //Single Threaded
	Matrix<int> a (num_rows,num_cols,5);
	Matrix<int> b (num_cols, num_rows,5); // <-- what the transposed matrix should look like
	get_start_time(start);
	Matrix<int> c = std::move(a.transpose());
	get_finish_time(finish);
	//confirm the transposition is accurate
	assert(c==b);
	//get duration of transposition operation
	elapsed = finish-start;
	std::cout << "SingleThreaded Transpose Time: " << elapsed.count() << std::endl;


 //Multithreaded
	get_start_time(start);
	c = std::move(a.transpose('m'));
	get_finish_time(finish);
	elapsed = finish-start;
	assert(c==b);
	std::cout << "MultiThreaded Transpose Time: " << elapsed.count() << std::endl;
}

//tests and prints some specific and easy to confrim examples
void test_mult(int mat_a_cols, int mat_a_rows, int mat_b_cols){
	std::chrono::high_resolution_clock::time_point start;
	std::chrono::high_resolution_clock::time_point finish;
	std::chrono::duration<double> elapsed;

	Matrix<int> a_mat(mat_a_rows, mat_a_cols,5 );
	Matrix<int> b_mat(mat_a_cols, mat_b_cols, 3);
	std::cout<< "----------------------------------" << std::endl;
	std::cout <<"Multiplying: " << "( " << mat_a_rows << "," << mat_a_cols <<" )" <<
		" x " << "( " << mat_a_cols << "," << mat_b_cols << " )" << std::endl;
	get_start_time(start);
	(a_mat*b_mat);
	get_finish_time(finish);
	elapsed = finish - start;
	std::cout << "SingleThread: " << elapsed.count() <<std::endl; 
	get_start_time(start);
	a_mat.fast_mult(b_mat);
	get_finish_time(finish);
	elapsed = finish - start;
	std::cout << "MultiThread: " << elapsed.count() << std::endl;

}
int main()
	//seed
{	srand(time(NULL));

	unsigned num_trials = 20; // <--number of times to run the transpose test,
	for(unsigned i = 0;i<num_trials;++i){
		test_transpose(rand() % 100,rand() % 100);
	}

	//simple example
	std::cout << "simple1" << std::endl;
	Matrix<int> a_ (4,3,2);
	Matrix<int> b_ (3,4,3);
	(a_*b_).print();
	
	for (unsigned i =0;i<num_trials;++i){
		test_mult(rand()%700,rand()%700,rand()%700); 
	}
	Matrix<int> m;

	//empty matrix transpose and print test (shouldn't print anything)
	m.transpose().print();
	m.transpose('m').print();
	std::cout << "tests are done (:" << std::endl;
	return 0;
}