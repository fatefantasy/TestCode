#include <iostream>
#include <utility>			// index_sequence
using namespace std;

template<int N>
struct SumN
{
	enum 
	{
		value = N + SumN<N-1>::value
	};
};

template<>
struct SumN<0>
{
	enum 
	{
		value = 0
	};
};

int main()
{
	cout<<SumN<10>::value<<endl;
	cout<<SumN<100>::value<<endl;
	cout<<SumN<1000>::value<<endl;	// fatal error: template instantiation depth exceeds maximum of 900 (use ‘-ftemplate-depth=’ to increase the maximum)
									// g++ template_depth.cpp -ftemplate-depth=10000: 编译通过;

	return 0;
}