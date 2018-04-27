#include <ttmath/ttmath.h>
#include <iostream>

// for convenience we're defining MyBig type
// this type has 2 words for its mantissa and 1 word for its exponent
// (on a 32bit platform one word means a word of 32 bits,
// and on a 64bit platform one word means a word of 64 bits)

// Big<exponent, mantissa>
typedef ttmath::Big<1,2> MyBig;

       
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
the result (on 32 bit platform):

Simple calculating
a = 123456.543456
b = 98767878.124322
a + b = 98891334.667778
a - b = -98644421.580866
a * b = 12193540837712.27076
a / b = 0.00124996654580957646
Calculating with a carry
a = 1.6248012560666408782e+646457012
b = 456.319999999999993
a + b = 1.6248012560666408782e+646457012
a - b = 1.6248012560666408782e+646457012
a * b = (carry)
a / b = 3.560661939136222174e+646457009
*/
