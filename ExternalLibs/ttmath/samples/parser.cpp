#include <ttmath/ttmath.h>
#include <iostream>


// for convenience we're defining MyBig type
// this type has 2 words for its mantissa and 1 word for its exponent
// (on a 32bit platform one word means a word of 32 bits,
// and on a 64bit platform one word means a word of 64 bits)
typedef ttmath::Big<1,2> MyBig;


int main()
{
ttmath::Parser<MyBig> parser;

// the sine function takes its parameter as being in radians,
// the product from the arcus tangent will be in radians as well
const char equation[] = " (34 + 24) * 123 - 34.32 ^ 6 * sin(2.56) - atan(10)";

    ttmath::ErrorCode err = parser.Parse(equation);

    if( err == ttmath::err_ok )
        std::cout << parser.stack[0].value << std::endl;
    else
        std::cout << "Error: "
                  << static_cast<int>(err)
                  << std::endl;
}

/*
the result (on 32 bit platform):
-897705014.525731067
*/


/*
the result (on 64 bit platform):
-897705014.5257310676097719585259773124
*/
