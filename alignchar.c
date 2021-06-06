/*
File: alignchar.c
License: BSD 2-Clause License

Copyright 2021 Costava

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

////////////////////////////////////////////////////////////////////////////////

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

////////////////////////////////////////////////////////////////////////////////

// Stringify.
#define STR(s) #s
// Expand token then stringify.
#define XSTR(s) STR(s)

// Capacity of buffer used for lines.
// Lines too long to fit into this buffer are left unchanged.
#define BUF_CAP 2048

#define VERSION_STR "0.2.0"

// When output mode is to modify the input file in place,
//  rename the input file to this name and read from it.
// If program outputs changed file to original input file path successfully,
//  then delete this file.
// If there is an error outputting, this file gets left behind (good).
#define INPUT_PATH_RENAMED "~alignchar_input_file_backup!!!"

////////////////////////////////////////////////////////////////////////////////

// Maybe char pointer
struct maybe_char_ptr {
	bool exists;
	char *value;
};

// How to handle output
enum output_mode {
	OUTPUT_MODE_UNSET = 0,
	OUTPUT_MODE_OUTPUT = 1,  // Output to a file
	OUTPUT_MODE_IN_PLACE = 2 // Modify input file
};

////////////////////////////////////////////////////////////////////////////////

// Get char from fp and populate out with that char.
// If error, print to stderr and exit.
// Return true if out populated else return false (end-of-file was reached).
// out is unchanged if false returned.
bool try_fgetc(FILE *const fp, char *const out) {
	const int ch = fgetc(fp);

	if (ch == EOF) {
		int ferr = ferror(fp);

		if (ferr != 0) {
			perror("fgetc error");
			exit(1);
		}

		// Reached end-of-file.
		return false;
	}

	*out = (char)ch;
	return true;
}

// fputc but calls perror and non-zero exits if error.
void checked_fputc(const char ch, FILE *const stream) {
	const int code = fputc(ch, stream);

	if (code != ch) {
		perror("fputc error");
		exit(1);
	}
}

// Read from file into buf of capacity buf_cap until target is found.
// num_written is set to the number of characters written into buf (not
//  including the null-terminator).
// If target is found, it does get written into buf.
// buf will always get null-terminated.
// If buf_cap is zero, prints to stderr and exits.
// On file error, prints to stderr and exits.
// Returns RTC_SUCCESS if target found.
// Returns RTC_EOF_REACHED if EOF reached before target found.
// Returns RTC_BUF_FULL if buf filled before target found.
#define RTC_SUCCESS 0
#define RTC_EOF_REACHED 1
#define RTC_BUF_FULL 2
uint8_t read_through_char(FILE *const file, char *const buf,
	const size_t buf_cap, const char target, size_t *const num_written)
{
	*num_written = 0;

	if (0 == buf_cap) {
		fprintf(stderr, "%s: Zero-capacity buffer was given.\n", __func__);
		exit(1);
	}
	else if (1 == buf_cap) {
		buf[0] = '\0';
		return RTC_BUF_FULL;
	}

	while (true) {
		char ch;
		if (try_fgetc(file, &ch)) {
			buf[*num_written] = ch;
			*num_written += 1;

			if (target == ch) {
				buf[*num_written] = '\0';
				return RTC_SUCCESS;
			}
			else if ((buf_cap - 1) == *num_written) {
				buf[*num_written] = '\0';
				return RTC_BUF_FULL;
			}
			else if (*num_written >= buf_cap) {
				// Either no room for '\0' or overflowed past end.
				fprintf(stderr, "%s: Impossible buf overflow.\n", __func__);
				exit(1);
			}
		}
		else {
			// EOF was reached.
			// The target char was never found.
			buf[*num_written] = '\0';
			return RTC_EOF_REACHED;
		}
	}

	fprintf(stderr, "%s: Unreachable.\n", __func__);
	exit(1);
}

// Copy from in into out until target found. Target gets copied.
// Return true if target found.
// Return false if EOF reached before target found.
// Prints to stderr and non-zero exits if file error.
bool transfer_through_char(FILE *const in, FILE *const out, const char target)
{
	char ch;
	while (try_fgetc(in, &ch)) {
		checked_fputc(ch, out);

		if (target == ch) {
			return true;
		}
	}

	return false;
}

// Return the width of line if tabs are tab_width wide.
// Returns wrong answer if line is wider than SIZE_MAX columns.
size_t get_line_width(const char *line, const size_t tab_width) {
	size_t width = 0;

	while ('\0' != line[0] && '\n' != line[0]) {
		if ('\t' == line[0]) {
			width += tab_width;
		}
		else {
			width += 1;
		}

		// Move pointer to next char.
		line += 1;
	}

	return width;
}

// Write buf_len number of chars from buf into file.
// Print to stderr and non-zero exit if error.
void ensure_fwriten(FILE *const file, const char *const buf,
	const size_t buf_len)
{
	const size_t num_written = fwrite(buf, sizeof(char), buf_len, file);

	if (num_written != buf_len) {
		fprintf(stderr, "%s: Expected: %ld Actual %ld\n", __func__, buf_len,
			num_written);

		exit(1);
	}
}

// Print help to stdout.
// Return 0 if successful.
// Return 1 if error printing.
int print_help(void) {
	const int code = fputs(
"alignchar: For the given input file, for each line that ends in the target\n"
"           character (Default: '\\'), align the target character to the\n"
"           target column position (Default: 80. First column is 1) using the\n"
"           fill character (Default: ' '). Non-matching lines, lines of\n"
"           length " XSTR(BUF_CAP)
                        " and greater, and lines where the target character\n"
"           falls on or after the target column position are unchanged.\n"
"\n"
"Usage examples:\n"
"  alignchar [options] -i <input file> -o <output file>\n"
"  alignchar [options] -i <input file> --in-place\n"
"\n"
"An input file must be specified (-i or --input).\n"
"Either an output file must be specified (-o or --output)\n"
"    or --in-place must be specified meaning modify the input file."
"\n"
"Options may be given in any order.\n"
"\n"
"Options:\n"
"  --help               Stop parsing options, print help, exit(0)\n"
"  --version            Stop parsing options, print version, exit(0)\n"
"\n"
"  -i, --input <path>   Specify input file (required)\n"
"  -o, --output <path>  Specify output file\n"
"                       (mutually exclusive with --in-place)\n"
"                       Do NOT specify the same path as for input\n"
"                       (use --in-place instead)\n"
"  --in-place           Modify input file\n"
"                       (mutually exclusive with -o, --output)\n"
"  -c, --char <char>    Specify the character to be aligned (Default: '\\')\n"
"  -p, --position <n>   Specify the column to align the character to\n"
"                       (Default: 80)\n"
"  -f, --fill <char>    Specify the fill character (Default: ' ')\n"
"  -t, --tab-width <n>  Specify tab width (Default: 4)\n"
"\n"
	, stdout);

	if (code != EOF) {
		// Printed successfully.
		return 0;
	}

	// There was an error printing.
	return 1;
}

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[]) {
	char target_char = '\\';
	// First column is 1.
	size_t target_pos = 80;
	char fill_char = ' ';
	// The tab width value to use when calculating line width.
	size_t tab_width = 4;

	struct maybe_char_ptr maybe_input_path = {false};

	const char *output_path = NULL;
	enum output_mode output_mode = OUTPUT_MODE_UNSET;

	// Parse command line arguments.
	for (int i = 1; i < argc; i += 1) {
		if (strcmp(argv[i], "--help") == 0) {
			exit(print_help());
		}
		else if (strcmp(argv[i], "--version") == 0) {
			const int code = fputs(VERSION_STR, stdout);

			if (code != EOF) {
				exit(0);
			}
			else {
				exit(1);
			}
		}
		else if (
			(strcmp(argv[i], "-i")      == 0) ||
			(strcmp(argv[i], "--input") == 0)
		) {
			if (i == (argc - 1)) {
				fprintf(stderr, "Error: The path to the input file must be "
					"after %s\n", argv[i]);
				exit(1);
			}

			if (maybe_input_path.exists) {
				fprintf(stderr, "Error: Only specify input file once. "
					"Current input file path: %s", maybe_input_path.value);
				exit(1);
			}

			maybe_input_path = (struct maybe_char_ptr){true, argv[i + 1]};

			// Jump over the input path.
			i += 1;
		}
		else if (
			(strcmp(argv[i], "-o")       == 0) ||
			(strcmp(argv[i], "--output") == 0)
		) {
			switch (output_mode) {
				case OUTPUT_MODE_UNSET: break; // Expected.
				case OUTPUT_MODE_OUTPUT:
				{
					fprintf(stderr, "Error: Only specify output file once. "
						"Current output file path: %s\n", output_path);
					exit(1);
				}
				case OUTPUT_MODE_IN_PLACE:
				{
					fprintf(stderr, "Error: Do not specify both %s and "
						"--in-place. Instead, specify exactly one of them.\n",
						argv[i]);
					exit(1);
				}
				default:
				{
					fprintf(stderr, "Error: Unknown output_mode value (when "
						"handling output file option): %d. "
						"This should NEVER happen.\n", output_mode);
					exit(1);
				}
			}

			if (i == (argc - 1)) {
				fprintf(stderr, "Error: The path to the output file must be "
					"after %s\n", argv[i]);
				exit(1);
			}

			output_path = argv[i + 1];
			output_mode = OUTPUT_MODE_OUTPUT;

			// Jump over output path.
			i += 1;
		}
		else if (strcmp(argv[i], "--in-place") == 0) {
			switch(output_mode) {
				case OUTPUT_MODE_UNSET: break; // Expected.
				case OUTPUT_MODE_OUTPUT:
				{
					fprintf(stderr, "Error: Do not specify both --in-place and "
						"output file (-o or --output). Instead, specify "
						"exactly one of them.\n");
					exit(1);
				}
				case OUTPUT_MODE_IN_PLACE:
				{
					fprintf(stderr, "Error: Do not specify --in-place more "
						"than once.\n");
					exit(1);
				}
				default:
				{
					fprintf(stderr, "Error: Unknown output_mode value (when "
						"handling option --in-place): %d. "
						"This should NEVER happen.\n", output_mode);
					exit(1);
				}
			}

			output_mode = OUTPUT_MODE_IN_PLACE;
		}
		else if (
			(strcmp(argv[i], "-c")     == 0) ||
			(strcmp(argv[i], "--char") == 0)
		) {
			if (i == (argc - 1)) {
				fprintf(stderr, "Error: Must specify target char "
					"after %s\n", argv[i]);
				exit(1);
			}

			const char *const char_str = argv[i + 1];

			if (strlen(char_str) != 1) {
				fprintf(stderr, "Error: Pass exactly one character to %s\n",
					argv[i]);
				exit(1);
			}

			target_char = char_str[0];

			// Jump over target char.
			i += 1;
		}
		else if (
			(strcmp(argv[i], "-p")         == 0) ||
			(strcmp(argv[i], "--position") == 0)
		) {
			if (i == (argc - 1)) {
				fprintf(stderr, "Error: Must specify target column "
					"after %s\n", argv[i]);
				exit(1);
			}

			const char *const pos_str = argv[i + 1];

			errno = 0;
			const long long val = strtoll(pos_str, NULL, 10);

			if (errno != 0) {
				fprintf(stderr, "Error: Failed to parse column position from "
					"\"%s\" as long long.\n", pos_str);
				exit(1);
			}

			if (val <= 0 || val >= BUF_CAP) {
				fprintf(stderr, "Error: Column position must be between "
					"0 and %d\n", BUF_CAP);
				exit(1);
			}

			target_pos = (size_t)val;

			// Jump over position.
			i += 1;
		}
		else if (
			(strcmp(argv[i], "-f")     == 0) ||
			(strcmp(argv[i], "--fill") == 0)
		) {
			if (i == (argc - 1)) {
				fprintf(stderr, "Error: Must specify fill char "
					"after %s\n", argv[i]);
				exit(1);
			}

			const char *const char_str = argv[i + 1];

			if (strlen(char_str) != 1) {
				fprintf(stderr, "Error: Pass exactly one character to %s\n",
					argv[i]);
				exit(1);
			}

			fill_char = char_str[0];

			// Jump over fill char.
			i += 1;
		}
		else if (
			(strcmp(argv[i], "-t")          == 0) ||
			(strcmp(argv[i], "--tab-width") == 0)
		) {
			if (i == (argc - 1)) {
				fprintf(stderr, "Error: Must specify tab width "
					"after %s\n", argv[i]);
				exit(1);
			}

			const char *const width_str = argv[i + 1];

			errno = 0;
			const long long val = strtoll(width_str, NULL, 10);

			if (errno != 0) {
				fprintf(stderr, "Error: Failed to parse tab width from "
					"\"%s\" as long long.\n", width_str);
				exit(1);
			}

			if (val < 0) {
				fprintf(stderr, "Error: Given tab width (%lld) must not be "
					"negative.\n", val);
				exit(1);
			}

			// If long long type larger than size_t,
			//  check that the parsed val does not overflow size_t.
			// long long is signed and size_t is unsigned,
			//  so we know conversion is safe otherwise.
			if (sizeof(long long) > sizeof(size_t)) {
				if (val > ((long long)SIZE_MAX)) {
					fprintf(stderr, "Error: Given tab width (%lld) overflows "
						"SIZE_MAX (%zu). Specify a smaller tab width.\n",
						val, SIZE_MAX);
					exit(1);
				}
			}

			tab_width = (size_t)val;

			// Jump over tab width.
			i += 1;
		}
		else {
			fprintf(stderr, "Error: Unrecognized arg: %s\n", argv[i]);
			exit(1);
		}
	}

	// Perform some final validation of inputs.

	if (!maybe_input_path.exists) {
		fprintf(stderr, "Error: You need to specify the input file using "
			"-i or --input option.\n");
		exit(1);
	}

	const char *input_path = maybe_input_path.value;

	switch (output_mode) {
		case OUTPUT_MODE_UNSET:
		{
			fprintf(stderr, "Error: You must either specify an output file "
				"(using -o or --output options) or specify modifying the input "
				"file in-place (--in-place)\n");
			exit(1);
		}
		case OUTPUT_MODE_OUTPUT: break;
		case OUTPUT_MODE_IN_PLACE: break;
		default:
		{
			fprintf(stderr, "Error: Unknown output_mode value (after "
				"parsing args): %d. "
				"This should NEVER happen.\n", output_mode);
			exit(1);
		}
	}

	// Open the input and output files.

	if (output_mode == OUTPUT_MODE_IN_PLACE) {
		// We are not going to modify the input file in place.
		// We are going to output to its path.
		// We need to move it/make a copy first.

		const int rename_code = rename(input_path, INPUT_PATH_RENAMED);

		if (rename_code != 0) {
			fprintf(stderr, "Error: Failed to move input file at: \"%s\" to "
				"the backup file path: \"%s\"\n",
				input_path, INPUT_PATH_RENAMED);
			exit(1);
		}

		output_path = input_path;
		input_path = INPUT_PATH_RENAMED;
	}

	FILE *const input = fopen(input_path, "rb");
	if (input == NULL) {
		fprintf(stderr, "Error: Failed to open input file: %s\n",
			input_path);
		exit(1);
	}

	FILE *const output = fopen(output_path, "wb");
	if (output == NULL) {
		fprintf(stderr, "Error: Failed to open output file: %s\n",
			output_path);
		exit(1);
	}

	// Begin reading input and outputting.

	while (true) {
		char buf[BUF_CAP];
		size_t buf_len;
		const uint8_t result = read_through_char(input, buf, BUF_CAP, '\n',
			&buf_len);

		if (result == RTC_SUCCESS || result == RTC_EOF_REACHED) {
			if (buf_len == 1) {
				// Blank line (only '\n').
				// We still need to write it out though.
				ensure_fwriten(output, buf, buf_len);

				if (result == RTC_EOF_REACHED) {
					// All lines handled.
					break;
				}

				continue;
			}

			// If the last char before the '\n' is target_char
			if (buf[buf_len - 2] == target_char) {
				size_t line_width = get_line_width(buf, tab_width);

				if (line_width >= target_pos) {
					// Nothing for us to do except write out line.
					ensure_fwriten(output, buf, buf_len);
				}
				else {
					// Align the target_char to target_pos position.

					// Write out line except for target_char and '\n'
					ensure_fwriten(output, buf, buf_len - 2);

					// (line_width - 1) because we should not count the
					//  target_char that we did not print above.
					// Assume: target_pos >= 1
					for (size_t i = line_width - 1; i < target_pos - 1;
					     i += 1)
					{
						checked_fputc(fill_char, output);
					}

					checked_fputc(target_char, output);
					checked_fputc('\n', output);
				}
			}
			else {
				// Simply write line out.
				ensure_fwriten(output, buf, buf_len);
			}

			if (result == RTC_EOF_REACHED) {
				// All lines handled.
				break;
			}
		}
		else if (result == RTC_BUF_FULL) {
			// Line is too long.
			// We just write it out and walk past the rest of it.
			ensure_fwriten(output, buf, buf_len);

			if (!transfer_through_char(input, output, '\n')) {
				// EOF reached. All done.
				break;
			}
		}
		else {
			fprintf(stderr, "Unknown RTC error: %d\n", result);
			exit(1);
		}
	}

	// Close input and output files.

	const int input_fclose_code = fclose(input);
	if (input_fclose_code != 0) {
		fprintf(stderr, "Failed to properly close input file: %s\n",
			input_path);
	}

	const int output_fclose_code = fclose(output);
	if (output_fclose_code != 0) {
		fprintf(stderr, "Failed to properly close output file: %s\n",
			output_path);
	}

	// Try to delete the backup file only if it closed properly.
	if (output_mode == OUTPUT_MODE_IN_PLACE && input_fclose_code == 0) {
		const int remove_code = remove(INPUT_PATH_RENAMED);

		if (remove_code != 0) {
			// Something did not go quite right.
			return 1;
		}
	}

	if (input_fclose_code != 0 || output_fclose_code != 0) {
		// Something did not go quite right.
		return 1;
	}

	return 0;
}
