#include <cstdint>

#include <xpdev/threadwrap.h>

int main()
{
	protected_uint64_t value;

	protected_uint64_init(&value, UINT64_C(0));
	protected_uint64_set(&value, UINT64_C(1));
	return protected_uint64_value(value) == UINT64_C(1) ? 0 : 1;
}
