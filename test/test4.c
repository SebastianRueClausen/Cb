/*
{
	float i = 1.0;

	while (123 * 2 >= 3)
	{
		if (i) {
			i = &i;	
		}
		else {
			i = 3 * 3;
		}
	}
}
*/

{
	int b;
	int i = 10;

	while (i < 10) {
		b = i;
		++i;
	}

	for (i = 0; i < 2; ++i) {
		b = i;
	}

	if (1) {
		b = i;
	} else {
		b = i;
	}
	
}
