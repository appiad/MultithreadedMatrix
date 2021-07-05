#ifndef __jq_h__
#define __jq_h__
#include "Job.h"
#include <queue>
#include <functional>
#include <mutex>

/*	Thread-safe queue where objects of type Job will be stored and accessed by
	a ThreadPool.
*/
template <class T> class Matrix;

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
	{
	}
	

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
#endif