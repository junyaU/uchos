#include <cstring>
#include <libs/user/console.hpp>
#include <libs/user/file.hpp>

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
	if (IS_ERR(fd)) {
		// Try to create the file
		fd = fs_create(filename);
		if (IS_ERR(fd)) {
			printu("Failed to open or create file: %s (%d)", filename,
				   static_cast<int>(fd));
			return 0;
		}
	}

	// Write text to file
	size_t text_len = strlen(text);
	ssize_t written = fs_write(fd, text, text_len);

	if (IS_ERR(written)) {
		printu("Failed to write to file: %s (%d)", filename,
			   static_cast<int>(written));
		fs_close(fd);
		return 0;
	}

	// Write newline
	const char newline[] = "\n";
	if (IS_ERR(fs_write(fd, newline, 1))) {
		printu("Failed to write newline to %s", filename);
	}

	fs_close(fd);

	printu("Written %d bytes to %s", static_cast<int>(written) + 1, filename);

	return 0;
}
