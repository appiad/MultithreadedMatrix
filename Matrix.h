#ifndef __Matrix_h__
#define __Matrix_h__
#include <thread>
#include <condition_variable>
#include <vector>
#include <string>
#include <mutex>
#include <iostream>
/* Author: appiad
	Code Sample - Matrix Multiplication and Transposition 





*/
//
class ThreadJoiner;
template <class T> class Matrix;



//Wrapper for a vector of threads to ensure all threads in the vector are 
//joined before the vector (and therefore the threads) are destroyed
class ThreadJoiner{
public:
	explicit ThreadJoiner(std::vector<std::thread>& threads) :
		m_threads(threads) {}

	~ThreadJoiner(){
		for (unsigned long i = 0;i<m_threads.size();++i){
			if (m_threads[i].joinable()){
				m_threads[i].join();
			}
		}
	}

private:
	std::vector<std::thread>& m_threads;
};


template <class T> class Matrix{
public:
	//CONSTRUCTORS AND ASSIGNMENT OPERATOR
	Matrix();                               //default
	Matrix(const Matrix& other);            //copy
	Matrix (unsigned rows, unsigned cols);  //construct with preset dimensions
	Matrix& operator=(const Matrix& other); //assignment operator

	//ACCESSORS 
	unsigned numRows() const;
	unsigned numCols() const;

	//MODIFIERS
	void transpose();

	//OPERATORS
	const Matrix operator*(const Matrix& other) const ;
private:
	std::vector<T> m_rows;
	std::vector<T> m_cols;
	//Mutable to allow const Matrix& other in operator= and 
	//operator* to have its mutex locked during operations
	
	mutable std::mutex matrix_lck; //mutable to allow other 
	

};	

 





#endif