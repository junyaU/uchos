#include <libs/user/console.hpp>
#include <libs/user/file.hpp>

// Simple strlen implementation
static size_t my_strlen(const char* str)
{
	size_t len = 0;
	while (str[len] != '\0') {
		len++;
	}
	return len;
}

int main(int argc, char** argv)
{
	if (argc < 3) {
		printu("Usage: write <file> <text>");
		return 0;
	}

	const char* filename = argv[1];
	const char* text = argv[2];
	
	// Open or create file
	fd_t fd = fs_open(filename, 0);
	if (fd < 0) {
		// Try to create the file
		fd = fs_create(filename);
		if (fd < 0) {
			printu("Failed to open or create file: %s", filename);
			return 0;
		}
	}
	
	// Write text to file
	size_t text_len = my_strlen(text);
	size_t written = fs_write(fd, text, text_len);
	
	if (written == 0) {
		printu("Failed to write to file: %s", filename);
		fs_close(fd);
		return 0;
	}
	
	// Write newline
	const char newline[] = "\n";
	fs_write(fd, newline, 1);
	
	fs_close(fd);
	
	printu("Written %d bytes to %s", written + 1, filename);
	
	return 0;
}