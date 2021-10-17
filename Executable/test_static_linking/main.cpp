

// you can define here on in your cmake file
#define VISUS_STATIC_LIB 1
#include <Visus/IdxDataset.h>

int main(void)
{
	auto dataset = Visus::LoadDataset("tmp/tutorial_1/visus.idx");
	return 0;
}

