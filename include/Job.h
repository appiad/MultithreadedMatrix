#ifndef __j_h__
#define __j_h__
#include <functional>

/*	stores a function of type 
	std::function<void(Matrix<T>*&,Matrix<T>*&,Matrix<T>*&, unsigned)>
	and its 4 arguments to be used in a JobQueue 
*/ 
template <class T> class Matrix;
template <class T>
class Job{
public:
	
	Job (std::function<void(Matrix<T>*&, Matrix<T>*&, Matrix<T>*&, unsigned)> func, 
		Matrix<T>* obj_ptr, Matrix<T>* ptr, Matrix<T>* ptr_other, unsigned func_arg2)
		:
		m_obj_ptr(obj_ptr),	m_func(func), m_ptr_matrix(ptr), 
		m_ptr_other_matrix(ptr_other), m_func_arg2(func_arg2)
	{
	}

	Matrix<T>* m_obj_ptr; //first arg of m_func
	std::function<void(Matrix<T>*&, Matrix<T>*&, Matrix<T>*&, unsigned)> m_func;
	Matrix<T>* m_ptr_matrix; //second arg of m_func
	Matrix<T>* m_ptr_other_matrix; //third arg of m_func
	unsigned m_func_arg2;     //fourth arg of m_func
	
};
#endif