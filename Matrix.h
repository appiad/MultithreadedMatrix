#ifndef __Matrix_h__
#define __Matrix_h__
#include <thread>
#include <condition_variable>
#include <vector>
#include <cstdlib>
#include <queue>
#include <functional>
#include <mutex>
#include <atomic>
#include <iostream>
#include "Job.h"
#include "JobQueue.h"
#include "ThreadPool.h"

/* 
Build Instuctions: g++ main_matrix.cpp -std="c++11" -pthread

	Note: JobQueue and ThreadPool are modified versions of Anthony William's
	ThreadSafeQueue and ThreadPool in his book C++ Concurrency in Action
	Practical Multithreading


*/
//FORWARD DECLARATIONS
template <class T> void 
call_transpose_one(Matrix<T>*& obj, Matrix<T>*& result, Matrix<T>*& other, unsigned row_index );
template <class T> void
call_mult_one(Matrix<T>*& obj, Matrix<T>*& result, Matrix<T>*& rhs, unsigned row_index);


/*
	Class capable of representing a Matrix of different types. Provides abiltiy
	to use multiple threads when performing operations to ideally speed
	up execution time. Provides threadsafe operations on Matrix. 

*/
template <class T> class Matrix{
public:
	typedef unsigned int size_type;

	//CONSTRUCTORS AND ASSIGNMENT OPERATORS
	Matrix();                               //default
	Matrix(const Matrix& other);            //copy
	Matrix(Matrix&& other);                 //move constructor
	Matrix (size_type num_rows, size_type num_cols);//Deafult Fill Constructor
	//Fill Constructor with fill_val
	Matrix (size_type num_rows, size_type num_cols, const T& fill_val);  
	Matrix& operator=(const Matrix& other); //copy assignment operator
	Matrix& operator=(Matrix&& other);      //move assignment operator
	bool operator== (const Matrix<T>& rhs) const;

	//ACCESSORS 
	void print() const;
	size_type numRows() const {
		std::lock_guard<std::mutex> lck (m_matrix_mtx);
		return m_num_rows;
	}
	size_type numCols() const {
		std::lock_guard<std::mutex> lck (m_matrix_mtx);
		return m_num_cols;
	}
	
	//OPERATIONS
	void push_row(std::vector<T>& a_row);
	Matrix operator*(const Matrix& other) const ;
	Matrix fast_mult( Matrix& other);
	Matrix transpose() const ;
	Matrix transpose( const char& type);
	
private:
	std::vector<std::vector<T> > m_data; 
	size_type m_num_rows;
	size_type m_num_cols;
	//Mutable to allow const functions to lock/unlock it on const objects
	mutable std::mutex m_matrix_mtx;

	//HELPERS
	void mult_one( Matrix*& result,  Matrix*& rhs,size_type row_index);
	void transpose_one(Matrix*& result, size_type row_index ) ;
	friend void call_transpose_one<T>(Matrix<T>*& obj, 
		Matrix<T>*& result, Matrix<T>*& other, unsigned row_index);	
	friend void call_mult_one<T>(Matrix<T>*& obj, Matrix<T>*& result, 
		Matrix<T>*& rhs, size_type row_index);
};	

//Default Constructor: creates empty Matrix
template <class T> 
Matrix<T>::Matrix() :
	m_num_rows(0),
	m_num_cols (0)
{
}

//Copy Constructor
template <class T> 
Matrix<T>::Matrix(const Matrix& other){
	std::lock_guard<std::mutex> other_lck (other.m_matrix_mtx);
	m_data(other.m_data);
	m_num_rows = other.m_num_rows;
	m_num_cols = other.m_num_cols;
}   

//Move Constructor
template <class T>
Matrix<T>::Matrix(Matrix&& other){
	std::lock_guard<std::mutex> other_lck(other.m_matrix_mtx);
	m_data = other.m_data;
	m_num_rows = other.m_num_rows;
	m_num_cols = other.m_num_cols;
	other.m_data = std::vector<std::vector<T> >();
	other.m_num_rows = 0;
	other.m_num_cols = 0;
}

//Default Fill Constructor (No val provided)
template <class T>
Matrix<T>::Matrix(size_type num_rows, size_type num_cols){
	m_data = std::vector<std::vector<T> >(num_rows,std::vector<T>(num_cols) );
	m_num_rows = num_rows;
	m_num_cols = num_cols;
}

//Fill Constructor: fills with preset row and column size and fill values
template <class T>
Matrix<T>::Matrix(size_type num_rows, size_type num_cols, const T& fill_val){
	m_data = std::vector<std::vector<T> >(num_rows,std::vector<T>( num_cols,fill_val) );
	m_num_rows = num_rows;
	m_num_cols = num_cols;
}

//Copy Assignment Operator
template <class T>
Matrix<T>& Matrix<T>::operator=(const Matrix& other){
	if (this != &other){
		std::lock(m_matrix_mtx,other.m_matrix_mtx);
		std::lock_guard<std::mutex> this_lck (m_matrix_mtx,std::adopt_lock);
		std::lock_guard<std::mutex> other_lck (other.m_matrix_mtx,std::adopt_lock);
		m_data = other.m_data;
		m_num_rows = other.m_num_rows;
		m_num_cols = other.m_num_cols;
	}
	return *this;
}
//Move Assignment Operator
template <class T>
Matrix<T>& Matrix<T>::operator=(Matrix&& other){
	if (this != &other){
		std::lock(m_matrix_mtx,other.m_matrix_mtx);
		std::lock_guard<std::mutex> this_lck (m_matrix_mtx,std::adopt_lock);
		std::lock_guard<std::mutex> other_lck (other.m_matrix_mtx,std::adopt_lock);
		m_data = other.m_data;
		m_num_rows = other.m_num_rows;
		m_num_cols = other.m_num_cols;
		other.m_data = std::vector<std::vector<T> >();
		other.m_num_rows = 0;
		other.m_num_cols = 0;
	}
	return *this;
}

//Prints out each row of the Matrix.
template <class T>
void Matrix<T>::print() const{
	std::lock_guard<std::mutex> mtx_lck (m_matrix_mtx);
	for (size_type i=0;i<m_num_rows;++i){
		std::cout << "R" << i  << ":" << std::endl << "   ";
		for (size_type j=0;j<m_num_cols;++j){
			std::cout << m_data[i][j] << " " ;
		} 
		std::cout << std::endl;
	}
}

//Enters a new row into the Matrix
//Returns error if row doesn't have compatible number of columns
template <class T>
void Matrix<T>::push_row(std::vector<T>& a_row){
	std::lock_guard<std::mutex> this_lck(m_matrix_mtx);
	if ((m_num_cols == 0 && m_num_cols == 0) || m_num_cols == a_row.size()){
		m_data.push_back(a_row);
		++m_num_rows;
	}
	else{
		std::cerr <<"Unable to push row onto Matrix" << std::endl;
		throw;
	}
}

template <class T> 
bool Matrix<T>::operator==( const Matrix<T>& rhs) const{
	if (this == &rhs)
		return true;
	std::lock(m_matrix_mtx, rhs.m_matrix_mtx);
	std::lock_guard<std::mutex> this_lck (m_matrix_mtx,std::adopt_lock);
	std::lock_guard<std::mutex> rhs_lck (rhs.m_matrix_mtx,std::adopt_lock);

	if (m_num_cols != rhs.m_num_cols || m_num_rows != rhs.m_num_rows){
		return false;
	}

	for(unsigned i = 0;i<m_num_rows;++i){
		for (unsigned j=0;j<m_num_cols;++j){
			if (m_data[i][j] != rhs.m_data[i][j]){
				return false;
			}
		}
	}
	return true;
}

template <class T>
Matrix<T> Matrix<T>::operator*(const Matrix& other) const {
	if (this == &other){
		std::cerr << "Incompatible: cannot multiply self" << std::endl;
		throw;
	}
	std::lock(m_matrix_mtx, other.m_matrix_mtx);
	std::lock_guard<std::mutex> this_lck (m_matrix_mtx, std::adopt_lock);
	std::lock_guard<std::mutex> other_lck (other.m_matrix_mtx, std::adopt_lock);

	//confirm matrices are appropriate size
	if (m_num_cols != other.m_num_rows){
		std::cerr << "Incompatible matrices given to multiply" << std::endl;
		throw;
	}
	Matrix result (m_num_rows, other.m_num_cols);
	for (size_type i = 0;i<m_num_rows;++i){
		for (size_type j = 0;j<other.m_num_cols;++j){
			T sum = T();
			for (size_type k=0;k<other.m_num_rows;++k){
				sum += (m_data[i][k] * other.m_data[k][j]);
			}
				result.m_data[i][j] = sum;
		}
	}
	return result;
}

//Multiplies the matrices using multiple threads to increase efficiency
template <class T>
Matrix<T> Matrix<T>::fast_mult( Matrix& other) {
	if (this == &other){
			std::cerr << "Incompatible: cannot multiply self" << std::endl;
			throw;	
		}

	std::lock(m_matrix_mtx, other.m_matrix_mtx);
	std::lock_guard<std::mutex> this_lck (m_matrix_mtx, std::adopt_lock);
	std::lock_guard<std::mutex> other_lck (other.m_matrix_mtx, std::adopt_lock);

	//confirm matrices are appropriate size
	if (m_num_cols != other.m_num_rows){
		std::cerr << "Incompatible matrices given to multiply" << std::endl;
		throw;
	}

	Matrix result (m_num_rows, other.m_num_cols);
	Matrix* ptr_result = &result;
	Matrix* ptr_other = &other;
	JobQueue<T> job_queue;
	//create a Job targeting each row of this matrix
	for (size_type i = 0;i<m_num_rows;++i){
		std::function<void(Matrix*&, Matrix*&, Matrix*&, unsigned)> f (call_mult_one<T>);
		job_queue.push(Job<T>(f,this, ptr_result, ptr_other, i));
	}
	std::condition_variable notify_when_finished;
	std::mutex this_thread_mtx;
	std::unique_lock<std::mutex> this_thread_lck (this_thread_mtx) ;
	ThreadPool<T> pool (job_queue, this_thread_mtx, notify_when_finished );
	//wait until we're notifed the thread pool is done doing work 
	while(!pool.isDone()){
		notify_when_finished.wait(this_thread_lck);
	}
	this_thread_lck.unlock();
	pool.join_all();
	return result;

}

/*
	Helper function that takes a single row (row_index) of this Matrix and 
	multiplies it with each column of the rhs Matrix. Stores the result of the
	operation in the result pointer Matrix. Used in a JobQueue to give each
	thread a row to target. 
*/
template <class T>
void Matrix<T>::mult_one( Matrix*& result, Matrix*& rhs, size_type row_index) {
	for (size_type i = 0;i<rhs->m_num_cols;++i){
		T sum = T();
		for (size_type j=0;j<rhs->m_num_rows;++j){
			sum += (m_data[row_index][j] * (rhs->m_data[j][i]));
		}
		result->m_data[row_index][i] = sum;
	}
}


/*
	Returns a tranposed version of the current Matrix using a single threaded
	approach.
*/
template <class T> 
Matrix<T> Matrix<T>::transpose() const {
	Matrix<T> transposed (m_num_cols,m_num_rows);
	for (size_type i = 0;i<m_num_rows;++i){
		for (size_type j=0;j<m_num_cols;++j){
			(transposed.m_data)[j][i] = this->m_data[i][j];
		}
	}
	return transposed;
}

/* 	
	Returns a transposed version of the current Matrix using a multi threaded
	apporach. Argument must be char 'm' to run or else
	an excpetion is thrown.
*/
template <class T> 
Matrix<T> Matrix<T>::transpose(const char& type)  {
	if (type == 'm'){
		std::lock_guard<std::mutex> mtx_lck (m_matrix_mtx);
		//create an empty matrix with a switched number of rows and columns as
		//this matrix
		Matrix<T> new_matrix (m_num_cols,m_num_rows);
		Matrix<T>* ptr_new_matrix = &new_matrix;
		Matrix<T>* ptr_unused = nullptr;
		JobQueue<T> job_queue;
		for (size_type i=0;i<m_num_rows;++i){
			std::function<void(Matrix<T>*&,Matrix<T>*&,Matrix<T>*&, unsigned)> f 
				(call_transpose_one<T>);
			job_queue.push(Job<T>(f, this, ptr_new_matrix, ptr_unused, i));
		}
		std::condition_variable notify_when_finished;
		std::mutex this_thread_mtx;
		std::unique_lock<std::mutex> this_thread_lck (this_thread_mtx) ;
		ThreadPool<T> pool (job_queue, this_thread_mtx, notify_when_finished );
		//wait until we're notifed the thread pool is done doing work 
		while(!pool.isDone()){
			notify_when_finished.wait(this_thread_lck);
		}
		pool.join_all();
		return new_matrix;
	}
	else{
		std::cerr << "Incorrect usage of multithreaded transpose" << std::endl;
		throw;
	}
}


/*
	Transposes a single row (row_index) of this Matrix and stores it in the 
	result Matrix. Used in JobQueue and ThreadPool to give each 
	thread a row to transpose.

*/
template <class T>
void Matrix<T>::transpose_one(Matrix<T>*& result, size_type row_index )  {
	for(size_type i= 0;i<m_num_cols;++i){
		(result->m_data)[i][row_index] = this->m_data[row_index][i];
	}
}

/*
	Function that calls transpose_one with given arguments.
	Used by each thread in ThreadPool to call the member function.
	Note that Matrix<T>*& other is unused. This was necessary to allow
	Matrix multiplication and transpostion use the same ThreadPool as
	the multiplication requires an extra Matrix<T>*& object.
*/ 
template <class T>  
void call_transpose_one(Matrix<T>*& obj, Matrix<T>*& result, Matrix<T>*& other, unsigned row_index ) {
	obj->transpose_one(result, row_index);
}

/*
	Function that calls mult_one with given arguments.
	Used by each thread in ThreadPool to call the member function.
*/
template <class T>
void call_mult_one(Matrix<T>*& obj, Matrix<T>*& result , Matrix<T>*& rhs, unsigned row_index ){
	obj->mult_one( result,rhs, row_index);
}


#endif