#include <ttmath/ttmath.h>
#include <iostream>


void SimpleCalculating(const ttmath::Int<2> & a, const ttmath::Int<2> & b)
{
	std::cout << "Simple calculating" << std::endl;
	std::cout << "a = " << a << std::endl;
	std::cout << "b = " << b << std::endl;
	std::cout << "a + b = " << a+b << std::endl;
	std::cout << "a - b = " << a-b << std::endl;
	std::cout << "a * b = " << a*b << std::endl;
	std::cout << "a / b = " << a/b << std::endl;
}


void CalculatingWithCarry(const ttmath::Int<2> & a, const ttmath::Int<2> & b)
{
ttmath::Int<2> atemp;

	std::cout << "Calculating with a carry" << std::endl;
	std::cout << "a = " << a << std::endl;
	std::cout << "b = " << b << std::endl;

	atemp = a;
	if( !atemp.Add(b) )
		std::cout << "a + b = " << atemp << std::endl;
	else
		std::cout << "a + b = (carry) " << atemp << std::endl;

	atemp = a;
	if( !atemp.Sub(b) )
		std::cout << "a - b = " << atemp << std::endl;
	else
		std::cout << "a - b = (carry) " << atemp << std::endl;
		
	atemp = a;
	if( !atemp.Mul(b) )
		std::cout << "a * b = " << atemp << std::endl;
	else
		std::cout << "a * b = (carry: the result is too big) " << std::endl;
		// it have no sense to print 'atemp' (it's undefined)
		
	atemp = a;
	if( !atemp.Div(b) )
		std::cout << "a / b = " << atemp << std::endl;
	else
		std::cout << "a / b = (division by zero) " << std::endl;
	
}

int main()
{
// on 32bit platforms: 'a' and 'b' have 2-words (two 32bit words)
// it means a,b are from <-2^63, 2^63 - 1>
ttmath::Int<2> a,b;
	
	// conversion from int
	a = 123456;
	
	// conversion from 'const char *'
	b = "98767878";
	
	SimpleCalculating(a,b);
	
	// 'a' will have the max value which can be held in this type
	a.SetMax();
	
	// conversion from 'int'
	b = 10;
	
	CalculatingWithCarry(a,b);
}

/*
the result (on 32 bit platform):

Simple calculating
a = 123456
b = 98767878
a + b = 98891334
a - b = -98644422
a * b = 12193487146368
a / b = 0
Calculating with a carry
a = 9223372036854775807
b = 10
a + b = (carry) -9223372036854775799
a - b = 9223372036854775797
a * b = (carry) the result is too big) 
a / b = 922337203685477580
*/
