#include <ttmath/ttmath.h>
#include <iostream>


void SimpleCalculating(const ttmath::UInt<2> & a, const ttmath::UInt<2> & b)
{
	std::cout << "Simple calculating" << std::endl;
	std::cout << "a = " << a << std::endl;
	std::cout << "b = " << b << std::endl;
	std::cout << "a + b = " << a+b << std::endl;
	std::cout << "a - b = " << a-b << std::endl;
	std::cout << "a * b = " << a*b << std::endl;
	std::cout << "a / b = " << a/b << std::endl;
}

			
void CalculatingWithCarry(const ttmath::UInt<2> & a, const ttmath::UInt<2> & b)
{
ttmath::UInt<2> atemp;

	std::cout << "Calculating with a carry" << std::endl;
	std::cout << "a = " << a << std::endl;
	std::cout << "b = " << b << std::endl;

	atemp = a;
	if( !atemp.Add(b) )
		std::cout << "a + b = " << atemp << std::endl;
	else
		// if there was a carry then atemp.Add(...) would have returned 1
		std::cout << "a + b = (carry: the result is too big) " << atemp << std::endl;

	atemp = a;
	if( !atemp.Sub(b) )
		std::cout << "a - b = " << atemp << std::endl;
	else
		std::cout << "a - b = (carry: 'a' was smaller than 'b') " << atemp << std::endl;
		
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
// it means a,b are from <0, 2^64 - 1>
ttmath::UInt<2> a,b;
	
	// conversion from 'const char *'
	a = "123456";
	
	// conversion from int
	b = 9876;
	
	SimpleCalculating(a,b);
	
	// 'a' will have the max value which can be held in this type
	a.SetMax();
	
	// conversion from 'int'
	b = 5;
	
	CalculatingWithCarry(a,b);
}

/*
the result (on 32 bit platform):

Simple calculating
a = 123456
b = 9876
a + b = 133332
a - b = 113580
a * b = 1219251456
a / b = 12
Calculating with a carry
a = 18446744073709551615
b = 5
a + b = (carry: the result is too big) 4
a - b = 18446744073709551610
a * b = (carry: the result is too big) 
a / b = 3689348814741910323
*/
