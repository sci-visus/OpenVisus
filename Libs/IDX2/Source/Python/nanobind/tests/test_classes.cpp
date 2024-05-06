#include <nanobind/nanobind.h>
#include <nanobind/trampoline.h>
#include <nanobind/operators.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/pair.h>
#include <memory>
#include <cstring>

namespace nb = nanobind;
using namespace nb::literals;

static int default_constructed = 0, value_constructed = 0, copy_constructed = 0,
           move_constructed = 0, copy_assigned = 0, move_assigned = 0,
           destructed = 0;

struct Struct;
std::unique_ptr<Struct> struct_tmp;

struct Struct {
    int i = 5;

    Struct() { default_constructed++; }
    Struct(int i) : i(i) { value_constructed++; }
    Struct(const Struct &s) : i(s.i) { copy_constructed++; }
    Struct(Struct &&s) noexcept : i(s.i) { s.i = 0; move_constructed++; }
    Struct &operator=(const Struct &s) { i = s.i; copy_assigned++; return *this; }
    Struct &operator=(Struct &&s) noexcept { std::swap(i, s.i); move_assigned++; return *this; }
    ~Struct() { destructed++; }

    int value() const { return i; }
    void set_value(int value) { i = value; }

    static int static_test(int) { return 1; }
    static int static_test(float) { return 2; }

    static Struct* create_take() { return new Struct(10); }
    static Struct  create_move() { return Struct(11); }
    static Struct* create_copy() { return struct_tmp.get(); }
    static Struct* create_reference() { return struct_tmp.get(); }
    Struct &self() { return *this; }
};

struct PairStruct {
    Struct s1;
    Struct s2;
};

struct Big {
    char data[1024];
    Big() { memset(data, 0xff, 1024); }
};

struct alignas(128) BigAligned {
    char data[128];
    BigAligned() {
        if (((uintptr_t) data) % 128)
            throw std::runtime_error("data is not aligned!");
        memset(data, 0xff, 128);
    }
};

struct Animal {
    virtual ~Animal() = default;
    virtual std::string name() const { return "Animal"; }
    virtual std::string what() const = 0;
    virtual void void_ret() { }
};

struct StaticProperties {
    static int value;
};

struct StaticProperties2 : StaticProperties { };

int StaticProperties::value = 23;

static Py_ssize_t sq_length_dummy(PyObject *) {
    return 123;
}

NB_MODULE(test_classes_ext, m) {
    struct_tmp = std::make_unique<Struct>(12);

    auto cls = nb::class_<Struct>(m, "Struct", "Some documentation")
        .def(nb::init<>())
        .def(nb::init<int>())
        .def("value", &Struct::value)
        .def("set_value", &Struct::set_value, "value"_a)
        .def("self", &Struct::self, nb::rv_policy::none)
        .def("none", [](Struct &) -> const Struct * { return nullptr; })
        .def_static("static_test", nb::overload_cast<int>(&Struct::static_test))
        .def_static("static_test", nb::overload_cast<float>(&Struct::static_test))
        .def_static("create_move", &Struct::create_move)
        .def_static("create_reference", &Struct::create_reference,
                    nb::rv_policy::reference)
        .def_static("create_copy", &Struct::create_copy,
                    nb::rv_policy::copy)
        .def_static("create_take", &Struct::create_take);


    if (!nb::type<Struct>().is(cls))
        nb::detail::raise("type lookup failed!");

    nb::class_<PairStruct>(m, "PairStruct")
        .def(nb::init<>())
        .def_readwrite("s1", &PairStruct::s1)
        .def_readwrite("s2", &PairStruct::s2);

    m.def("stats", []{
        nb::dict d;
        d["default_constructed"] = default_constructed;
        d["value_constructed"] = value_constructed;
        d["copy_constructed"] = copy_constructed;
        d["move_constructed"] = move_constructed;
        d["copy_assigned"] = copy_assigned;
        d["move_assigned"] = move_assigned;
        d["destructed"] = destructed;
        return d;
    });

    m.def("reset", []() {
        default_constructed = 0;
        value_constructed = 0;
        copy_constructed = 0;
        move_constructed = 0;
        copy_assigned = 0;
        move_assigned = 0;
        destructed = 0;
    });

    // test06_big

    nb::class_<Big>(m, "Big")
        .def(nb::init<>());

    nb::class_<BigAligned>(m, "BigAligned")
        .def(nb::init<>());

    // test09_trampoline
    // test10_trampoline_failures

    struct PyAnimal : Animal {
        NB_TRAMPOLINE(Animal, 3);

        PyAnimal() {
            default_constructed++;
        }

        ~PyAnimal() override {
            destructed++;
        }

        std::string name() const override {
            NB_OVERRIDE(std::string, Animal, name);
        }

        std::string what() const override {
            NB_OVERRIDE_PURE(std::string, Animal, what);
        }

        void void_ret() override {
            NB_OVERRIDE(void, Animal, void_ret);
        }
    };

    struct Dog : Animal {
        Dog(const std::string &s) : s(s) { }
        std::string name() const override { return "Dog"; }
        std::string what() const override { return s; }
        std::string s;
    };

    struct Cat : Animal {
        Cat(const std::string &s) : s(s) { }
        std::string name() const override { return "Cat"; }
        std::string what() const override { return s; }
        std::string s;
    };

    auto animal = nb::class_<Animal, PyAnimal>(m, "Animal")
        .def(nb::init<>())
        .def("name", &Animal::name)
        .def("what", &Animal::what);

    nb::class_<Dog, Animal>(m, "Dog")
        .def(nb::init<const std::string &>());

    nb::class_<Cat>(m, "Cat", animal)
        .def(nb::init<const std::string &>());

    m.def("go", [](Animal *a) {
        return a->name() + " says " + a->what();
    });

    m.def("void_ret", [](Animal *a) { a->void_ret(); });

    m.def("call_function", [](nb::handle h) {
        return h(1, 2, "hello", true, 4);
    });

    m.def("call_method", [](nb::handle h) {
        return h.attr("f")(1, 2, "hello", true, 4);
    });

    // test11_large_pointers
    m.def("i2p", [](uintptr_t x) { return (Cat *) x; }, nb::rv_policy::reference);
    m.def("p2i", [](Cat *x) { return (uintptr_t) x; });

    // test12_implicitly_convertible
    struct A { int a; };
    struct B { int b; };
    struct B2 : B { B2(int i) : B{i} { } };
    struct C { int c; };

    struct D {
        D(const A &a) : value(a.a + 10) { }
        D(const B *b) : value(b->b + 100) { }
        D(C c) : value(c.c + 1000) { }
        D(int d) : value(d + 10000) { }
        D(float) : value(0) { throw std::runtime_error("Fail!"); }
        int value;
    };

    nb::class_<A>(m, "A")
        .def(nb::init<int>());

    nb::class_<B>(m, "B")
        .def(nb::init<int>());

    nb::class_<B2, B>(m, "B2")
        .def(nb::init<int>());

    nb::class_<C>(m, "C")
        .def(nb::init<int>());

    nb::class_<D>(m, "D")
        .def(nb::init_implicit<const A &>())
        .def(nb::init_implicit<const B *>())
        .def(nb::init_implicit<int>())
        .def(nb::init_implicit<float>())
        .def_readwrite("value", &D::value);

    m.def("get_d", [](const D &d) { return d.value; });

    struct Int {
        int i;
        Int operator+(Int o) const { return {i + o.i}; }
        Int operator-(float j) const { return {int(i - j)}; }
        Int &operator+=(Int o) {
            i += o.i;
            return *this;
        }
    };

    // test13_operators
    nb::class_<Int>(m, "Int")
        .def(nb::init<int>())
        .def(nb::self + nb::self)
        .def(nb::self += nb::self)
        .def(nb::self - float())
        .def("__repr__", [](const Int &i) { return std::to_string(i.i); });


    // test15: Test nb::keep_alive feature
    m.def(
        "keep_alive_arg", [](nb::handle, nb::handle ret) { return ret; },
        nb::keep_alive<0, 1>());

    m.def(
        "keep_alive_ret", [](nb::handle, nb::handle ret) { return ret; },
        nb::keep_alive<1, 0>());

    // test17_name_qualname_module()
    m.def("f", []{});
    struct MyClass { struct NestedClass { }; };
    nb::class_<MyClass> mcls(m, "MyClass");
    nb::class_<MyClass::NestedClass> ncls(mcls, "NestedClass");
    mcls.def("f", []{});
    ncls.def("f", []{});

    // test18_static_properties
    nb::class_<StaticProperties>(m, "StaticProperties")
        .def_readwrite_static("value", &StaticProperties::value)
        .def_static("get", []{ return StaticProperties::value; } );

    nb::class_<StaticProperties2, StaticProperties>(m, "StaticProperties2");

    // test19_supplement
    struct ClassWithSupplement { };
    struct Supplement {
        uint8_t data[0xFF];
    };

    auto scls = nb::class_<ClassWithSupplement>(m, "ClassWithSupplement", nb::supplement<Supplement>())
        .def(nb::init<>());

    Supplement &supplement = nb::type_supplement<Supplement>(scls);
    for (uint8_t i = 0; i < 0xFF; ++i)
        supplement.data[i] = i;

    m.def("check_supplement", [](nb::handle h) {
        if (nb::isinstance<ClassWithSupplement>(h)) {
            Supplement &s2 = nb::type_supplement<Supplement>(h.type());
            for (uint16_t i = 0; i < 0xFF; ++i) {
                if (s2.data[i] != i)
                    return false;
            }
            return true;
        }
        return false;
    });

    // test20_type_callback
    auto callback = [](PyType_Slot **s) noexcept {
        *(*s)++ = { Py_sq_length, (void *) sq_length_dummy };
    };

    struct ClassWithLen { };
    nb::class_<ClassWithLen>(m, "ClassWithLen", nb::type_callback(callback))
        .def(nb::init<>());

    // test21_low_level
    m.def("test_lowlevel", []() {
        nb::handle py_type = nb::type<Struct>();
        if (!(nb::type_check(py_type) &&
              nb::type_size(py_type) == sizeof(Struct) &&
              nb::type_align(py_type) == alignof(Struct) &&
              nb::type_info(py_type) == typeid(Struct)))
            throw std::runtime_error("Internal error!");

        nb::object py_inst = nb::inst_alloc(py_type);
        if (!(nb::inst_check(py_inst) && py_inst.type().is(py_type) &&
              !nb::inst_ready(py_inst)))
            throw std::runtime_error("Internal error! (2)");

        // Get a C++ pointer to the uninitialized instance data
        Struct *ptr = nb::inst_ptr<Struct>(py_inst);

        // Perform an in-place construction of the C++ object
        new (ptr) Struct(123);

        nb::inst_mark_ready(py_inst);
        if (!nb::inst_ready(py_inst))
            throw std::runtime_error("Internal error! (3)");

        nb::object py_inst_2 = nb::inst_alloc(py_type);
        if (nb::inst_ready(py_inst_2))
            throw std::runtime_error("Internal error! (4)");

        nb::inst_copy(py_inst_2, py_inst);
        if (!nb::inst_ready(py_inst_2))
            throw std::runtime_error("Internal error! (5)");

        nb::inst_destruct(py_inst);
        if (nb::inst_ready(py_inst))
            throw std::runtime_error("Internal error! (6)");

        nb::inst_move(py_inst, py_inst_2);
        if (!nb::inst_ready(py_inst))
            throw std::runtime_error("Internal error! (7)");

        return std::make_pair(py_inst, py_inst_2);
    });

    // test22_handle_t
    m.def("test_handle_t", [](nb::handle_t<Struct> h) { return borrow(h); });

    // test23_type_object_t
    m.def("test_type_object_t", [](nb::type_object_t<Struct> h) -> nb::object { return std::move(h); });

    // test24_none_arg
    m.def("none_0", [](Struct *s) { return s == nullptr; });
    m.def("none_1", [](Struct *s) { return s == nullptr; }, nb::arg());
    m.def("none_2", [](Struct *s) { return s == nullptr; }, nb::arg("arg"));
    m.def("none_3", [](Struct *s) { return s == nullptr; }, nb::arg().none());
    m.def("none_4", [](Struct *s) { return s == nullptr; }, nb::arg("arg").none());

    // test25_is_final
    struct FinalType { };
    nb::class_<FinalType>(m, "FinalType", nb::is_final())
        .def(nb::init<>());

    // test26_dynamic_attr
    struct StructWithAttr : Struct { };
    nb::class_<StructWithAttr, Struct>(m, "StructWithAttr", nb::dynamic_attr())
        .def(nb::init<int>());
}
