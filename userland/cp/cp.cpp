#include <cstdio>
#include <cstdlib>

extern "C" void main(int argc, char** argv)
{
	if (argc < 3) {
		printf("Usage: cp <source> <destination>\n");
		exit(1);
	}

	FILE* src = fopen(argv[1], "r");
	if (src == nullptr) {
		printf("Failed to open file: %s\n", argv[1]);
		exit(1);
	}

	FILE* dst = fopen(argv[2], "w");
	if (dst == nullptr) {
		printf("Failed to open file: %s\n", argv[2]);
		exit(1);
	}

	char buffer[256];
	size_t read_size = 0;
	while ((read_size = fread(buffer, 1, sizeof(buffer), src)) > 0) {
		const size_t written = fwrite(buffer, 1, read_size, dst);
		if (written != read_size) {
			printf("Failed to write to file: %s\n", argv[2]);
			exit(1);
		}
	}

	exit(0);
}