#include "shell.hpp"
#include <unistd.h>
#include <cstring>
#include <libs/user/file.hpp>
#include <libs/user/syscall.hpp>
#include "libs/common/types.hpp"
#include "terminal.hpp"

Shell::Shell() { memset(histories, 0, sizeof(histories)); }

void Shell::process_input(char* input, Terminal& term)
{
	if (strlen(input) == 0) {
		term.enable_input = true;
		return;
	}

	// Check for redirection
	char* redirect_pos = strchr(input, '>');
	char* redirect_file = nullptr;

	if (redirect_pos != nullptr) {
		// Null-terminate the command part
		*redirect_pos = '\0';

		// Skip spaces after '>'
		redirect_file = redirect_pos + 1;
		while (*redirect_file == ' ') {
			redirect_file++;
		}

		int len = strlen(redirect_file);
		while (len > 0 && redirect_file[len - 1] == ' ') {
			redirect_file[len - 1] = '\0';
			len--;
		}

		// Remove trailing spaces from command
		char* end = redirect_pos - 1;
		while (end >= input && *end == ' ') {
			*end = '\0';
			end--;
		}
	}

	const char* args = strchr(input, ' ');
	int command_length = strlen(input);
	if (args != nullptr) {
		command_length = args - input;
		++args;
	} else {
		args = "";
	}

	char command_name[command_length + 1];
	memcpy(command_name, input, command_length);
	command_name[command_length] = '\0';

	pid_t pid = sys_fork();
	if (pid == 0) {
		// Handle redirection in child process
		if (redirect_file != nullptr && strlen(redirect_file) > 0) {
			// Create or open the file
			fd_t file_fd = fs_create(redirect_file);
			if (file_fd < 0) {
				// Try opening existing file
				file_fd = fs_open(redirect_file, 0);
			}

			if (file_fd >= 0) {
				// Redirect stdout to file
				fs_dup2(file_fd, STDOUT_FILENO);
				fs_close(file_fd);
			}
		}

		error_t status = sys_exec(command_name, args);
		exit(status);
	}

	int child_status;
	sys_wait(&child_status);

	if (child_status != 0) {
		term.printf("%s : command not found\n", command_name);
		term.enable_input = false;
		return;
	}

	term.enable_input = false;
}
