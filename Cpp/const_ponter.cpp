#include <iostream>

int main()
{
	int i = 123;

	int * const ipc = &i;
	// ipc = &i;			// error: assignment of read-only variable ‘ipc’
	*ipc = 456;

	int const * icp = &i;
	icp = &i;
	// *icp = 456;			// error: assignment of read-only location ‘* icp’

	const int * cip = &i;
	cip = &i;
	// *cip = 456;			// error: assignment of read-only location ‘* cip’

	return 0;
}
