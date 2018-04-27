#include <ttmath/ttmath.h>
#include <iostream>


// this is a similar example to big.cpp
// but now we're using TTMATH_BITS() macro
// this macro returns how many words we need to store
// the given number of bits

// TTMATH_BITS(64) 
//   on a 32bit platform the macro returns 2 (2*32=64)
//   on a 64bit platform the macro returns 1

// TTMATH_BITS(128)
//   on a 32bit platform the macro returns 4 (4*32=128)
//   on a 64bit platform the macro returns 2 (2*64=128)

// Big<exponent, mantissa>
typedef ttmath::Big<TTMATH_BITS(64), TTMATH_BITS(128)> MyBig;

// consequently on a 32bit platform we define: Big<2, 4>
// and on a 64bit platform: Big<1, 2>
// and the calculations will be the same on both platforms       
       
       
void SimpleCalculating(const MyBig & a, const MyBig & b)
{
	std::cout << "Simple calculating" << std::endl;
	std::cout << "a = " << a << std::endl;
	std::cout << "b = " << b << std::endl;
	std::cout << "a + b = " << a+b << std::endl;
	std::cout << "a - b = " << a-b << std::endl;
	std::cout << "a * b = " << a*b << std::endl;
	std::cout << "a / b = " << a/b << std::endl;
}


void CalculatingWithCarry(const MyBig & a, const MyBig & b)
{
MyBig atemp;

	std::cout << "Calculating with a carry" << std::endl;
	std::cout << "a = " << a << std::endl;
	std::cout << "b = " << b << std::endl;

	atemp = a;
	if( !atemp.Add(b) )
		std::cout << "a + b = " << atemp << std::endl;
	else
		std::cout << "a + b = (carry)" << std::endl;
		// it have no sense to print 'atemp' (it's undefined)

	atemp = a;
	if( !atemp.Sub(b) )
		std::cout << "a - b = " << atemp << std::endl;
	else
		std::cout << "a - b = (carry)" << std::endl;
		
	atemp = a;
	if( !atemp.Mul(b) )
		std::cout << "a * b = " << atemp << std::endl;
	else
		std::cout << "a * b = (carry)" << std::endl;
		
		
	atemp = a;
	if( !atemp.Div(b) )
		std::cout << "a / b = " << atemp << std::endl;
	else
		std::cout << "a / b = (carry or division by zero) " << std::endl;
	
}


int main()
{
MyBig a,b;
	
	// conversion from 'const char *'
	a = "123456.543456";
	b = "98767878.124322";
	
	SimpleCalculating(a,b);
	
	// 'a' will have the max value which can be held in this type
	a.SetMax();
	
	// conversion from double
	b = 456.32;
	
	// Look at the value 'a' and the product from a+b and a-b
	// Don't worry this is the nature of floating point numbers
	CalculatingWithCarry(a,b);
}

/*
the result (the same on a 32 or 64bit platform):

Simple calculating
a = 123456.543456
b = 98767878.124322
a + b = 98891334.667778
a - b = -98644421.580866
a * b = 12193540837712.270763536832
a / b = 0.0012499665458095764605964485261668609133
Calculating with a carry
a = 2.34953455457111777368832820909595050034e+2776511644261678604
b = 456.3199999999999931787897367030382156
a + b = 2.34953455457111777368832820909595050034e+2776511644261678604
a - b = 2.34953455457111777368832820909595050034e+2776511644261678604
a * b = (carry)
a / b = 5.1488748127873374141170361292780486452e+2776511644261678601
*/
