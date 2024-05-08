#include <nanobind/nanobind.h>
#include <nanobind/stl/pair.h>

namespace nb = nanobind;
using namespace nb::literals;

int call_guard_value = 0;

struct my_call_guard {
    my_call_guard() { call_guard_value = 1; }
    ~my_call_guard() { call_guard_value = 2; }
};

NB_MODULE(test_functions_ext, m) {
    // Function without inputs/outputs
    m.def("test_01", []() { });

    // Simple binary function (via function pointer)
    auto test_02 = [](int j, int k) -> int { return j - k; };
    m.def("test_02", (int (*)(int, int)) test_02, "j"_a = 8, "k"_a = 1);

    // Simple binary function with capture object
    int i = 42;
    m.def("test_03", [i](int j, int k) -> int { return i + j - k; });

    // Large capture object requiring separate storage
    uint64_t k = 10, l = 11, m_ = 12, n = 13, o = 14;
    m.def("test_04", [k, l, m_, n, o]() -> int { return (int) (k + l + m_ + n + o); });

    // Overload chain with two docstrings
    m.def("test_05", [](int) -> int { return 1; }, "doc_1");
    m.def("test_05", [](float) -> int { return 2; }, "doc_2");

    /// Function raising an exception
    m.def("test_06", []() { throw std::runtime_error("oops!"); });

    /// Function taking some positional/keyword args and nb::[kw]args
    m.def("test_07", [](int, int, nb::args args, nb::kwargs kwargs) {
        return std::make_pair(args.size(), kwargs.size());
    });

    /// As above, but with nb::arg annotations
    m.def("test_07", [](int, int, nb::args args, nb::kwargs kwargs) {
        return std::make_pair(args.size(), kwargs.size());
    }, "a"_a, "b"_a, "myargs"_a, "mykwargs"_a);

    /// Test successful/unsuccessful tuple conversion
    m.def("test_tuple", []() { return nb::make_tuple("Hello", 123); });
    m.def("test_bad_tuple", []() { struct Foo{}; return nb::make_tuple("Hello", Foo()); });

    /// Perform a Python function call from C++
    m.def("test_call_1", [](nb::object o) { return o(1); });
    m.def("test_call_2", [](nb::object o) { return o(1, 2); });

    /// Test expansion of args/kwargs-style arguments
    m.def("test_call_extra", [](nb::object o, nb::args args, nb::kwargs kwargs) {
        return o(1, 2, *args, **kwargs, "extra"_a = 5);
    });

    /// Test list manipulation
    m.def("test_list", [](nb::list l) {
        int result = 0;
        for (size_t i = 0; i < l.size(); ++i)
            result += nb::cast<int>(l[i]);
        l[2] = 123;
        l.append(result);
    });

    /// Test tuple manipulation
    m.def("test_tuple", [](nb::tuple l) {
        int result = 0;
        for (size_t i = 0; i < l.size(); ++i)
            result += nb::cast<int>(l[i]);
        return result;
    });

    /// Test call_guard feature
    m.def("test_call_guard", []() {
        return call_guard_value;
    }, nb::call_guard<my_call_guard>());

    m.def("call_guard_value", []() { return call_guard_value; });

    m.def("test_release_gil", []() -> bool {
#if defined(Py_LIMITED_API)
        return false;
#else
        return PyGILState_Check();
#endif
    }, nb::call_guard<nb::gil_scoped_release>());

    m.def("test_print", []{
        nb::print("Test 1");
        nb::print(nb::str("Test 2"));
    });

    m.def("test_iter", [](nb::object in) {
        nb::list l;
        for (nb::handle h : in)
            l.append(h);
        return l;
    });

    m.def("test_iter_tuple", [](nb::tuple in) {
        nb::list l;
        for (nb::handle h : in)
            l.append(h);
        return l;
    });

    m.def("test_iter_list", [](nb::list in) {
        nb::list l;
        for (nb::handle h : in)
            l.append(h);
        return l;
    });

    // Overload chain with a raw docstring that has precedence
    m.def("test_08", [](int) -> int { return 1; }, "ignored");
    m.def("test_08", [](float) -> int { return 2; }, nb::raw_doc("raw"));

    // Manual type check
    m.def("test_09", [](nb::type_object t) -> bool { return t.is(&PyBool_Type); });

    // nb::dict iterator
    m.def("test_10", [](nb::dict d) {
        nb::dict result;
        for (auto [k, v] : d)
            result[k] = v;
        return result;
    });

    // Test implicit conversion of various types
    m.def("test_11_sl",  [](signed long x)      { return x; });
    m.def("test_11_ul",  [](unsigned long x)        { return x; });
    m.def("test_11_sll", [](signed long long x) { return x; });
    m.def("test_11_ull", [](unsigned long long x)   { return x; });

    // Test string caster
    m.def("test_12", [](const char *c) { return nb::str(c); });
    m.def("test_13", []() -> const char * { return "test"; });
    m.def("test_14", [](nb::object o) -> const char * { return nb::cast<const char *>(o); });

    // Test bytes type
    m.def("test_15", [](nb::bytes o) -> const char * { return o.c_str(); });
    m.def("test_16", [](const char *c) { return nb::bytes(c); });
    m.def("test_17", [](nb::bytes c) { return c.size(); });
    m.def("test_18", [](const char *c, int size) { return nb::bytes(c, size); });
}
