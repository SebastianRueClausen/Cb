typedef struct test_struct
{
	volatile long i;
	const short j;
	char c;

	union
	{
		int k;
		float l;
		double d;
	};

	enum
	{
		TEST1,
		TEST2
	};
} struct_t;


int main(int argc, char** argv)
{
	struct_t t;

	t.d = 1.2 / 2;

	return 1;
}
