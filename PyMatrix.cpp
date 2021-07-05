#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <vector>
#include "Matrix.h"
namespace py = pybind11;
PYBIND11_MODULE(MMatrix, m){

    py::class_<Matrix<int>>(m, "matrix")
        .def(py::init<>());

}

//class VList {
//    public:
//        std::vector<long long> list;
//        unsigned int len;
//        VList(int s) {
//            len = s;
//            for(int i=0; i<s; i++) list.push_back(0);
//        }
//        void append(long long val) {
//            len++;
//            list.push_back(val);
//        }
//        void mod_list() {
//            for(unsigned int i=0; i<len; i++) list[i] = (list[i]*list[i])-3;
//        }
//        void mod_index(unsigned int ind, long long val) {
//            list[ind] = val;
//        }
//};
//// below is the binding code! We specify the class and all its methods
//// vector_list should be same name as file (which we'll name vector_list.cpp)
//PYBIND11_MODULE(vector_list, m) {  //vector_list being the file name (without .cpp)
//    py::class_<VList>(m, "VList")
//        .def(py::init<int>())
//        .def("append", &VList::append)
//        .def("mod_list", &VList::mod_list)
//        .def("mod_index", &VList::mod_index)
//        .def_readwrite("list", &VList::list);  // accessing our vector 'list'
//}