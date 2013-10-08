#include <boost/python.hpp>

using namespace boost::python;

int add_five(int x) {
    return x + 5;
}

BOOST_PYTHON_MODULE(Pointless)
{
    def("add_five", add_five);
}

int main(int, char **)
{
  Py_Initialize(); 
  
    try {
        initPointless(); // initialize Pointless

        PyRun_SimpleString("import Pointless");
        PyRun_SimpleString("print Pointless.add_five(4)");
    } catch (error_already_set) {
        PyErr_Print();
    }

    Py_Finalize();
    return 0;
}