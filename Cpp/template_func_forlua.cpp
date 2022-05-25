/** 代码目的: 将方法封装为可供lua调用的方法；*/

#include <iostream>
#include <functional>		// std::function
#include <utility>			// index_sequence
#include <type_traits>		// remove_xxx
using namespace std;

// 伪lua相关定义;
struct lua_State {};
//
template<typename T>
T Read(lua_State* L, int Idx)   
{ 
	const char* i 	= is_same<T, int>::value 		? "int:" : "";
	const char* ir 	= is_same<T, int&>::value 		? "int&:" : "";
	const char* ic 	= is_same<T, const int>::value 	? "const int:" : "";
	const char* icr = is_same<T, const int&>::value ? "const int&:" : "";
	cout<<"read value: "<<Idx<<" :"<<i<<ir<<ic<<icr<<endl; return T(); 
}
template<>
int Read(lua_State*L, int Idx)  { cout<<"read int: "<<Idx<<endl; return 100+Idx; }
//
void Push(lua_State* L, int v)  { cout<<"push int: "<<v<<endl; }
template <typename T>
void Push(lua_State* L, T v)	{ cout<<"push value: "<<v<<endl; }
//
template<typename T>
bool Update(lua_State* L, int Idx, T v)	 			{ cout<<"update value["<<Idx<<"]: "<<v<<endl; return false; }
template<>			// int or const int
bool Update(lua_State* L, int Idx, int v)  			{ cout<<"update int ["<<Idx<<"]: "<<v<<endl; return true; }
template<>
bool Update(lua_State* L, int Idx, int& v)  		{ cout<<"update int& ["<<Idx<<"]: "<<v<<endl; return true; }
// template<>		// int or const int
// bool Update(lua_State* L, int Idx, const int v) 	{ cout<<"update const int ["<<Idx<<"]: "<<v<<endl; return true; }
template<>
bool Update(lua_State* L, int Idx, const int& v)  	{ cout<<"update const int& ["<<Idx<<"]: "<<v<<endl; return true; }
//
template<typename T, bool bIsRef = is_reference<T>::value, bool bIsConst = is_const<typename remove_reference<T>::type>::value>
struct FUpdateParam
{
	static int Update(lua_State* L, int idx, const T v)	{ cout<<"FUpdateParam: donot update"<<endl; return 0; }
};
template<typename T>
struct FUpdateParam<T, true, false>
{
	static int Update(lua_State* L, int idx, const T& v)	
	{ 
		cout<<"FUpdateParam: do update: \t\t";
		::Update<T>(L, idx, v);
		return 1;
	}
};
template<typename T>
struct FUpdateParam<T, true, true>
{
	static int Update(lua_State* L, int idx, const T& v){ cout<<"FUpdateParam: donot update: const T&"<<endl;	return 0; }
};
template<typename T>
struct FUpdateParam<T, false, false>
{
	static int Update(lua_State* L, int idx, const T& v){ cout<<"FUpdateParam: donot update: T"<<endl;	return 0; }
};
template<typename T>
struct FUpdateParam<T, false, true>
{
	static int Update(lua_State* L, int idx, const T& v){ cout<<"FUpdateParam: donot update: const T"<<endl;	return 0; }
};

// 
typedef std::function<int(lua_State*)> LuaFunction;
template<typename T>
struct TReadLuaType 
{ 
	typedef typename remove_reference<typename remove_const<T>::type>::type type_rc;	// 这个操作顺序会把 constT& 变为 constT
	typedef typename remove_const<typename remove_reference<T>::type>::type type_cr;	// 这个操作顺序才会 constT& 变为 T
};

// 
template<typename RetType, typename... ArgTypes, size_t... Idx>
int LuaFuncCallCFunc(lua_State* L, RetType(*f)(ArgTypes...), index_sequence<Idx...> idxs)
{
	// 1. 如果不考虑引用参数问题: 直接读参调用即可:
	// 1.1 直接Read需要为 T/ConstT& 做不同的特例化适配;
	// RetType Ret = f(Read<ArgTypes>(L, Idx)...);
	// 1.2 将参数类型做减法, 移除各种修饰:
	// RetType Ret = f(Read<typename TReadLuaType<ArgTypes>::type_cr>(L, Idx+1)...);

	// 2. 考虑引用参数: 
	// 2.1 直接调用会编译错误: error: cannot bind non-const lvalue reference of type ‘int&’ to an rvalue of type ‘int’
	// RetType Ret = f(Read<typename TReadLuaType<ArgTypes>::type_cr>(L, Idx+1)...);
	// 2.2 构建纯粹T类型临时参数
	tuple<typename TReadLuaType<ArgTypes>::type_cr...> params 
		= make_tuple(Read<typename TReadLuaType<ArgTypes>::type_cr>(L, Idx+1)...);
	RetType ret = f(get<Idx>(params)...);
	Push<RetType>(L, ret);

	// 3. 在2的基础上可以再继续对引用参数做处理:
	int Cnt = 1;
	int _[] = {0,  (Cnt+=(FUpdateParam<ArgTypes>::Update(L, Idx+1, get<Idx>(params))))...};
	return Cnt;
}
// 
template<typename RetType, typename... ArgTypes, size_t... Idx>
int LuaFuncCallCFunc(lua_State* L, std::function<RetType(ArgTypes...)> f, index_sequence<Idx...> idxs)
{
	// std::function 形式的参数: 内容同上;
	tuple<typename TReadLuaType<ArgTypes>::type_cr...> params 
		= make_tuple(Read<typename TReadLuaType<ArgTypes>::type_cr>(L, Idx+1)...);
	RetType ret = f(get<Idx>(params)...);
	Push<RetType>(L, ret);
	int Cnt = 1;
	int _[] = {0,  (Cnt+=(FUpdateParam<ArgTypes>::Update(L, Idx+1, get<Idx>(params))))...};
	return Cnt;
}
	
// 
template<typename RetType, typename... ArgTypes>
LuaFunction CovertToLuaFunc(RetType(*f)(ArgTypes...))
{
	return [=](lua_State* L)
	{
		return LuaFuncCallCFunc(L, f, make_index_sequence<sizeof...(ArgTypes)>{});
	};
}
template<typename RetType, typename... ArgTypes>
LuaFunction CovertToLuaFunc(std::function<RetType(ArgTypes...)> f)
{
	return [=](lua_State* L)
	{
		return LuaFuncCallCFunc(L, f, make_index_sequence<sizeof...(ArgTypes)>{});
	};
}

// 方法:
class FLib
{
public:
	static int FuncA(int i, const int ic, const int& icr)
	{
		cout<<"i:"<<i<<endl;		i = 201;
		cout<<"ic:"<<ic<<endl;	
		cout<<"icr:"<<icr<<endl;
		return 99;
	}
	static int FuncB(int i, int& ir, const int ic, const int& icr)
	{ 
		cout<<"i: "<<i<<endl;		i = 201;
		cout<<"ir: "<<ir<<endl;		ir = 202;
		cout<<"ic: "<<ic<<endl;
		cout<<"icr: "<<icr<<endl;
		return 99; 
	}
};

int main()
{
	lua_State* L = nullptr;
	int i = 0;
	cout<<"---- ---- ---- ---- is_const:"<<endl;
	cout<<std::boolalpha;
	cout<<"int: "<<is_const<int>::value<<endl;					// >> false
	cout<<"int&: "<<is_const<int&>::value<<endl;				// >> false
	cout<<"const int: "<<is_const<const int>::value<<endl;		// >> true
	cout<<"const int&: "<<is_const<const int&>::value<<endl;	// >> false

	cout<<"---- ---- ---- ---- is_reference:"<<endl;
	cout<<std::boolalpha;
	cout<<"int: "<<is_reference<int>::value<<endl;				// >> false
	cout<<"int&: "<<is_reference<int&>::value<<endl;			// >> true
	cout<<"const int: "<<is_reference<const int>::value<<endl;	// >> false
	cout<<"const int&: "<<is_reference<const int&>::value<<endl;// >> true

	cout<<"---- ---- ---- ---- Update:"<<endl;			// 使用<int>&&屏蔽<const int>		// 屏蔽<int>使用<const int>
	Update<int>(L, 0, i);								// >> update int[0]: 0				// update const int [0]: 0
	Update<int&>(L, 0, i);								// >> update int& [0]: 0			// ==
	Update<const int>(L, 0, i);							// >> update value[0]: 0			// ==
	Update<const int&>(L, 0, i);						// >> update const int& [0]: 0		// ==
														// TODO: <const int> ...
	cout<<"---- ---- ---- ---- FUpdateParam:"<<endl;	// >> bIsConst=is_const<T>								// bIsConst=is_const<remove_ref<T>>
	FUpdateParam<int>::Update(L, 0, i);					// >> FUpdateParam: donot update: T						// ==
	FUpdateParam<int&>::Update(L, 0, i);				// >> FUpdateParam: do update: update value[0]: 0		// ==
	FUpdateParam<const int>::Update(L, 0, i);			// >> FUpdateParam: donot update: const T				// ==
	FUpdateParam<const int&>::Update(L, 0, i);			// >> FUpdateParam: do update: update const int& [0]: 0 // FUpdateParam: donot update: const T&

	cout<<"---- ---- ---- ---- fa:"<<endl;
	auto fa = CovertToLuaFunc(FLib::FuncA);
	auto a = fa(L);
	cout<<"fa ret: "<<a<<endl;		
	
	cout<<"---- ---- ---- ---- fb:"<<endl;
	auto fb = CovertToLuaFunc(FLib::FuncB);
	auto b = fb(L);							// >> read int: 4
											// 	  read int: 3
											// 	  read int: 2
											// 	  read int: 1
											// >> i: 101
											//	  ir: 102
											//	  ic: 103
											//	  icr: 104
											// >> push value: 99
											// >> FUpdateParam: donot update: T
											// 	  FUpdateParam: do update: update int& [2]: 202
											//	  FUpdateParam: donot update: T
											// 	  FUpdateParam: donot update: const T&
	cout<<"fb ret: "<<b<<endl;				// >> fb ret: 2
											// TODO: <const T>类型还是丢了const...


	cout<<"---- ---- ---- ---- lambda:"<<endl;
	std::function<int(int, int)> l = 
		[](int i, int j)
		{
			cout<<"lambda i: "<<i<<endl;
			cout<<"lambda j: "<<j<<endl;
			return 33;
		};
	auto fl = CovertToLuaFunc(l);
	fl(L);
	/** 直接传递lambda编译错误:
	 * 	no matching function for call to ‘CovertToLuaFunc(main()::<lambda(int, int)>)’
	auto fl1 = CovertToLuaFunc(
		[](int i, int j)
		{
			cout<<"lambda i: "<<i<<endl;
			cout<<"lambda j: "<<j<<endl;
			return 33;
		});
	/**/
	/** 直接传递lambda编译错误: 添加类型说明 (vs可以识别编译通过, 但gcc还是错误)
	 * 	error: no matching function for call to ‘CovertToLuaFunc<int, int, int>(main()::<lambda(int, int)>)’
	auto fl1 = CovertToLuaFunc<int, int, int>(
		[](int i, int j)
		{
			cout<<"lambda i: "<<i<<endl;
			cout<<"lambda j: "<<j<<endl;
			return 33;
		});
	/**/

	return 0;
}

