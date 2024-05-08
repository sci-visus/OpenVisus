import test_functions_ext as t
import pytest

def test01_capture():
    # Functions with and without capture object of different sizes
    assert t.test_01() is None
    assert t.test_02(5, 3) == 2
    assert t.test_03(5, 3) == 44
    assert t.test_04() == 60


def test02_default_args():
    # Default arguments
    assert t.test_02() == 7
    assert t.test_02(7) == 6


def test03_kwargs():
    # Basic use of keyword arguments
    assert t.test_02(3, 5) == -2
    assert t.test_02(3, k=5) == -2
    assert t.test_02(k=5, j=3) == -2


def test04_overloads():
    assert t.test_05(0) == 1
    assert t.test_05(0.0) == 2


def test05_signature():
    assert t.test_01.__doc__ == 'test_01() -> None'
    assert t.test_02.__doc__ == 'test_02(j: int = 8, k: int = 1) -> int'
    assert t.test_05.__doc__ == (
        "test_05(arg: int, /) -> int\n"
        "test_05(arg: float, /) -> int\n"
        "\n"
        "Overloaded function.\n"
        "\n"
        "1. ``test_05(arg: int, /) -> int``\n"
        "\n"
        "doc_1\n"
        "\n"
        "2. ``test_05(arg: float, /) -> int``\n"
        "\n"
        "doc_2")

    assert t.test_07.__doc__ == (
        "test_07(arg0: int, arg1: int, /, *args, **kwargs) -> tuple[int, int]\n"
        "test_07(a: int, b: int, *myargs, **mykwargs) -> tuple[int, int]")

def test06_signature_error():
    with pytest.raises(TypeError) as excinfo:
        t.test_05("x", y=4)
    assert str(excinfo.value) == (
        "test_05(): incompatible function arguments. The "
        "following argument types are supported:\n"
        "    1. test_05(arg: int, /) -> int\n"
        "    2. test_05(arg: float, /) -> int\n\n"
        "Invoked with types: str, kwargs = { y: int }")


def test07_raises():
    with pytest.raises(RuntimeError) as excinfo:
        t.test_06()
    assert str(excinfo.value) == "oops!"


def test08_args_kwargs():
    assert t.test_07(1, 2) == (0, 0)
    assert t.test_07(a=1, b=2) == (0, 0)
    assert t.test_07(a=1, b=2, c=3) == (0, 1)
    assert t.test_07(1, 2, 3, c=4) == (1, 1)
    assert t.test_07(1, 2, 3, 4, c=5, d=5) == (2, 2)


def test09_maketuple():
    assert t.test_tuple() == ("Hello", 123)
    with pytest.raises(RuntimeError) as excinfo:
        assert t.test_bad_tuple()
    assert str(excinfo.value) == (
        "nanobind::detail::tuple_check(...): conversion of argument 2 failed!")


def test10_cpp_call_simple():
    result = []
    def my_callable(a, b):
        result.append((a, b))

    t.test_call_2(my_callable)
    assert result == [(1, 2)]

    with pytest.raises(TypeError) as excinfo:
        t.test_call_1(my_callable)
    assert "my_callable() missing 1 required positional argument: 'b'" in str(excinfo.value)
    assert result == [(1, 2)]


def test11_call_complex():
    result = []
    def my_callable(*args, **kwargs):
        result.append((args, kwargs))

    t.test_call_extra(my_callable)
    assert result == [
        ((1, 2), {"extra" : 5})
    ]

    result.clear()
    t.test_call_extra(my_callable, 5, 6, hello="world")
    assert result == [
      ((1, 2, 5, 6), {"extra" : 5, "hello": "world"})
    ]


def test12_list_tuple_manipulation():
    li = [1, 5, 6, 7]
    t.test_list(li)
    assert li == [1, 5, 123, 7, 19]

    tu = (1, 5, 6, 7)
    assert t.test_tuple(tu) == 19
    assert tu == (1, 5, 6, 7)


def test13_call_guard():
    assert t.call_guard_value() == 0
    assert t.test_call_guard() == 1
    assert t.call_guard_value() == 2
    assert not t.test_release_gil()


def test14_print(capsys):
    t.test_print()
    captured = capsys.readouterr()
    assert captured.out == "Test 1\nTest 2\n"


def test15_iter():
    assert t.test_iter(()) == []
    assert t.test_iter((1,)) == [1]
    assert t.test_iter((1, 2)) == [1, 2]
    assert t.test_iter((1, 2, 3)) == [1, 2, 3]


def test16_iter_tuple():
    assert t.test_iter_tuple(()) == []
    assert t.test_iter_tuple((1,)) == [1]
    assert t.test_iter_tuple((1, 2)) == [1, 2]
    assert t.test_iter_tuple((1, 2, 3)) == [1, 2, 3]


def test17_iter_tuple():
    assert t.test_iter_list([]) == []
    assert t.test_iter_list([1]) == [1]
    assert t.test_iter_list([1, 2]) == [1, 2]
    assert t.test_iter_list([1, 2, 3]) == [1, 2, 3]


def test18_raw_doc():
    assert t.test_08.__doc__ == 'raw'


def test19_type_check_manual():
    assert t.test_09.__doc__ == 'test_09(arg: type, /) -> bool'

    assert t.test_09(bool) is True
    assert t.test_09(int) is False
    with pytest.raises(TypeError) as excinfo:
        assert t.test_09(True)
    assert "incompatible function arguments" in str(excinfo.value)


def test20_dict_iterator():
    assert t.test_10({}) == {}
    assert t.test_10({1:2}) == {1:2}
    assert t.test_10({1:2, 3:4}) == {1:2, 3:4}
    assert t.test_10({1:2, 3:4, 'a': 'b'}) == {1:2, 3:4, 'a':'b'}


def test21_numpy_overloads():
    try:
        import numpy as np
    except ImportError:
        pytest.skip('numpy is missing')

    assert t.test_05(np.int32(0)) == 1
    assert t.test_05(np.float64(0.0)) == 2
    assert t.test_11_sl(np.int32(5)) == 5
    assert t.test_11_ul(np.int32(5)) == 5
    assert t.test_11_sll(np.int32(5)) == 5
    assert t.test_11_ull(np.int32(5)) == 5


def test22_string_return():
    assert t.test_12("hello") == "hello"
    assert t.test_13() == "test"
    assert t.test_14("abc") == "abc"


def test23_byte_return():
    assert t.test_15(b"abc") == "abc"
    assert t.test_16("hello") == b"hello"
    assert t.test_17(b"four") == 4
    assert t.test_17(b"\x00\x00\x00\x00") == 4
    assert t.test_18("hello world", 5) == b"hello"
