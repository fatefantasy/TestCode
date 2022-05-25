#include <iostream>
#include <functional>
#include <utility>
using namespace std;

template<typename RetType, typename... ArgTypes>
void WrapperCallFunc(std::function<RetType(ArgTypes...)> Func)
{
	Func(11, 22);
}

template<typename RetType, typename... ArgTypes>
std::function<void()> WrapperFunc(std::function<RetType(ArgTypes...)> Func)
{
	return [=]()
	{
		WrapperCallFunc(Func);
	}
}

int main()
{
	auto f1 = WrapperFunc<void, int, int>(
		[](int i1, int i2){
			cout<<i1<<endl;
			cout<<i1<<endl;
		}
	);

	return 0;
}