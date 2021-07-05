#ifndef __tp__h__
#define  __tp__h__
#include "JobQueue.h"
#include <vector>
#include <cstdlib>
#include <mutex>
#include <queue>
#include <functional>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <cstdlib>

/*	Creates threads to do the work stored in the JobQueue object 
	it is constructed with. Returns control to calling thread once all
	Jobs are completed. Calling thread is required to call
	join_all after being notified to awaken.

*/
template <class T> class Matrix;

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
#endif