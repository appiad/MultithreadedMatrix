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
/* Author: appiad
	testing
	Code Sample - Matrix Multiplication and Transposition 
Build Instuctions: g++ .cpp -std="c++11" -pthread

	Everything is done in a single .h file except for the 
	testing done in the main file. Each class will have their own 
	header and cpp file in a future revision.
	

	Note: JobQueue and ThreadPool are modified versions of Anthony William's
	ThreadSafeQueue and ThreadPool in his book C++ Concurrency in Action
	Practical Multithreading


*/
//FORWARD DECLARATIONS
template <class T> class Job;
template <class T> class JobQueue;
template <class T> class ThreadPool;
template <class T> class Matrix;
template <class T> void 
call_transpose_one(Matrix<T>*& obj, Matrix<T>*& result, Matrix<T>*& other, unsigned row_index );
template <class T> void
call_mult_one(Matrix<T>*& obj, Matrix<T>*& result, Matrix<T>*& rhs, unsigned row_index);




/*	stores a function of type 
	std::function<void(Matrix<T>*&,Matrix<T>*&,Matrix<T>*&, unsigned)>
	and its 4 arguments to be used in a JobQueue 
*/ 
template <class T>
class Job{
public:
	
	Job (std::function<void(Matrix<T>*&,Matrix<T>*&,Matrix<T>*&, unsigned)> func, 
		Matrix<T>* obj_ptr, Matrix<T>* ptr, Matrix<T>* ptr_other, unsigned func_arg2)
		:
		m_obj_ptr(obj_ptr),	m_func(func), m_ptr_matrix(ptr), 
		m_ptr_other_matrix(ptr_other), m_func_arg2(func_arg2)

		{}
	Matrix<T>* m_obj_ptr; //first arg of m_func
	std::function<void(Matrix<T>*&,Matrix<T>*&,Matrix<T>*&,unsigned)> m_func;
	Matrix<T>* m_ptr_matrix; //second arg of m_func
	Matrix<T>* m_ptr_other_matrix; //third arg of m_func
	unsigned m_func_arg2;     //fourth arg of m_func
	
};


/*	Thread-safe queue where objects of type Job will be stored and accessed by
	a ThreadPool.
*/
template <class T>
class JobQueue{
public:
	//Default Constructor
	JobQueue(){
		m_data_queue = 	std::queue<Job<T> >();
	}
	//Copy Constructor
	JobQueue(const JobQueue& other): 
		m_data_queue(other.m_data_queue)
	{}
	

	//Attempts to retrieve a Job's function and its arguments from the queue to passed 
	//parameters. If successful then returns true, else returns false. 
	bool try_pop(std::function<void(Matrix<T>*&,Matrix<T>*&,Matrix<T>*&, unsigned)>& function, 
		Matrix<T>*& obj_ptr, Matrix<T>*& ptr, Matrix<T>*& ptr_other,  unsigned& arg){

		std::lock_guard<std::mutex> queue_lck (m_queue_mtx) ;
		if (m_data_queue.empty()){
			return false;
		}
		//assign the function and its arguments to the passed in parameters
		Job<T> tmp = m_data_queue.front();
		function = tmp.m_func;
		obj_ptr = tmp.m_obj_ptr;
		ptr = tmp.m_ptr_matrix;
		ptr_other = tmp.m_ptr_other_matrix;
		arg = tmp.m_func_arg2;
		//remove the Job from the queue and return true to indicate success
		m_data_queue.pop();
		return true;
	}

	//Returns true if the queue is empty and false otherwise
	bool empty() const{
		std::lock_guard<std::mutex> queue_lck (m_queue_mtx) ;
		return m_data_queue.empty();
	}
	//Returns the size of the queue
	unsigned size() const{
		std::lock_guard<std::mutex> queue_lck (m_queue_mtx);
		return m_data_queue.size();
	}

	//Pushes Job onto the queue
	void push(Job<T> j){
		m_data_queue.push(j);
	}

private:
	std::queue<Job<T> > m_data_queue; //queue of jobs
	mutable std::mutex m_queue_mtx; 

};

/*	Creates threads to do the work stored in the JobQueue object 
	it is constructed with. Returns control to calling thread once all
	Jobs are completed. Calling thread is required to call
	join_all after being notified to awaken.

*/
template <class T>
class ThreadPool{

public:
	ThreadPool(JobQueue<T>& jq, std::mutex& mtx, 
		std::condition_variable& work_done_flag):

		m_job_queue(jq), m_calling_thread_mtx(mtx),  
		m_work_done_cv(work_done_flag), m_done(false), m_all_jobs_loaded(false),
		m_num_jobs_completed(0)

		{	
			m_num_jobs_assigned = m_job_queue.size();
			//create as many threads as this machine is capable of running concurrently
			const unsigned m_thread_count = std::thread::hardware_concurrency();
			try{
				for (unsigned i=0;i<m_thread_count;++i){
					m_threads.push_back(
						std::thread(&ThreadPool::worker_thread,this));	
				}
				assert(m_threads.size() == 4);
				m_all_jobs_loaded = true;
			}
			catch(...){
				std::cerr << "A problem occured in trying to create the ThreadPool"
					<< std::endl;
				throw;
			}
		}


	/*	Passed to wait function of main thread's condition variable to confirm 
		that it only wakes up when all threads in the pool are done working and 
		have been joined. This is to prevent a spurious wake-up. 
	*/ 
	bool isDone()  {return unsigned(m_done);}

	//Joins all joinable threads in the pool.
	void join_all(){
		assert(m_threads.size() == 4);
		for(unsigned i=0;i<m_threads.size();++i){
			if (m_threads[i].joinable() ){
				m_threads[i].join();
			}
		}
	}

	/* Each thread runs this function and continuously takes Jobs from 
	   the job queue and completes them until there are none left. At 
	   that point a single thread will wait for all other threads to return
	   from this function. That thread will than call notify_calling_thread()
	   before returning.
	*/ 
	void worker_thread(){
		while (!m_done){
			//only start once all jobs have been loaded into the queue
			if (m_all_jobs_loaded){
				std::function<void(Matrix<T>*&,Matrix<T>*&,Matrix<T>*&, unsigned)> task;
				Matrix<T>* obj_ptr;
				Matrix<T>* ptr_matrix;
				Matrix<T>* ptr_other_matrix;
				unsigned arg2;
				if (m_job_queue.try_pop(task, obj_ptr, ptr_matrix, ptr_other_matrix, arg2)){
					task(obj_ptr, ptr_matrix, ptr_other_matrix, arg2);
					m_num_jobs_completed += 1;;
				}
				//queue is empty therefore all jobs are done
				else{
					std::call_once(m_notify_of, 
						&ThreadPool::notify_calling_thread, this);
				}
			}
			else{
				std::this_thread::yield();
			}
		}
	}

	//Wakes up the thread that iniated the ThreadPool once ALL Jobs
	//have been assigned and have been comepletly finished.
	void notify_calling_thread(){
		while (1){
			//wait for any threads in the middle of doing a Job
			if (m_num_jobs_assigned == unsigned(m_num_jobs_completed)){
				break;
			}
			else{
				std::this_thread::yield();
			}
		}
		//wake the thread that initiated the ThreadPool.
		std::unique_lock<std::mutex> calling_thread_lck(m_calling_thread_mtx); 
		m_done = true;
		m_work_done_cv.notify_one();
	}

private:
	unsigned m_thread_count; //number of threads to create
	std::atomic<bool> m_done; //notifies all threads in pool all work is done
	std::vector<std::thread> m_threads;
	JobQueue<T> m_job_queue;
	//notifies main thread all work is done so it can wake and continue
	std::condition_variable& m_work_done_cv;
	std::once_flag m_notify_of;
	std::mutex& m_calling_thread_mtx; //mutex of the thread that iniated the pool
	std::atomic_bool m_all_jobs_loaded; //flag to signal threads may begin work
	std::atomic<unsigned> m_num_jobs_completed; //number of completed Jobs
	unsigned m_num_jobs_assigned; //number of Jobs the job_queue starts of with
};

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
{}

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