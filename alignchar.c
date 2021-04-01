/* MIT LICENSE
Copyright (c) 2021 Costava

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

///////////////////////////////////////////////////////////////////////////////

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

///////////////////////////////////////////////////////////////////////////////

#define TARGET_CHAR '\\'
// The char to use when aligning the TARGET_CHAR to the correct position.
#define FILL_CHAR ' '
// Code assumes value in range [1, SIZE_MAX - 3]
// 1 is first column.
#define TARGET_COLUMN 79
// The tab width value to use when calculating line width.
#define TAB_WIDTH 4

///////////////////////////////////////////////////////////////////////////////

// Buffer capacity
// Plus 1 for \n and plus 1 for \0
// Assume: TARGET_COLUMN <= (SIZE_MAX - 3)
//          Additional -1 so that the condition check in a for loop would be
//          able to have a value that exceeds BUF_CAP
#define BUF_CAP (TARGET_COLUMN + 2)

///////////////////////////////////////////////////////////////////////////////

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

///////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[]) {
	if (3 != argc) {
		fprintf(stderr,
			"USAGE: ./alignchar path/to/inputfile path/to/outputfile\n");

		exit(1);
	}

	#define INPUT_FILE_PATH argv[1]
	#define OUTPUT_FILE_PATH argv[2]

	FILE *const input = fopen(INPUT_FILE_PATH, "rb");
	if (NULL == input) {
		fprintf(stderr, "Failed to open input file: %s\n", INPUT_FILE_PATH);
		exit(1);
	}

	FILE *const output = fopen(OUTPUT_FILE_PATH, "wb");
	if (NULL == output) {
		fprintf(stderr, "Failed to open output file: %s\n", OUTPUT_FILE_PATH);
		exit(1);
	}

	while (true) {
		char buf[BUF_CAP];
		size_t buf_len;
		const uint8_t result = read_through_char(input, buf, BUF_CAP, '\n',
			&buf_len);

		if (RTC_SUCCESS == result || RTC_EOF_REACHED == result) {
			if (1 == buf_len) {
				// Blank line (only '\n').
				// We still need to write it out though.
				ensure_fwriten(output, buf, buf_len);

				if (RTC_EOF_REACHED == result) {
					// All lines handled.
					break;
				}

				continue;
			}

			// If the last char before the '\n' is TARGET_CHAR
			if (TARGET_CHAR == buf[buf_len - 2]) {
				size_t line_width = get_line_width(buf, TAB_WIDTH);

				// printf("line_width: %ld line: %s\n", line_width, buf);

				if (line_width >= TARGET_COLUMN) {
					// Nothing for us to do except write out line.
					ensure_fwriten(output, buf, buf_len);
				}
				else {
					// Align the TARGET_CHAR to TARGET_COLUMN position.

					// Write out line except for TARGET_CHAR and '\n'
					ensure_fwriten(output, buf, buf_len - 2);

					// (line_width - 1) because we should not count the
					//  TARGET_CHAR that we did not print above.
					// Assume: TARGET_COLUMN >= 1
					for (size_t i = line_width - 1; i < TARGET_COLUMN - 1;
					     i += 1)
					{
						checked_fputc(FILL_CHAR, output);
					}

					checked_fputc(TARGET_CHAR, output);
					checked_fputc('\n', output);
				}
			}
			else {
				// Simply write line out.
				ensure_fwriten(output, buf, buf_len);
			}

			if (RTC_EOF_REACHED == result) {
				// All lines handled.
				break;
			}
		}
		else if (RTC_BUF_FULL == result) {
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
	if (0 != input_fclose_code) {
		fprintf(stderr, "Failed to properly close input file: %s\n",
			INPUT_FILE_PATH);
	}

	const int output_fclose_code = fclose(output);
	if (0 != output_fclose_code) {
		fprintf(stderr, "Failed to properly close output file: %s\n",
			OUTPUT_FILE_PATH);
	}

	if (0 != input_fclose_code || 0 != output_fclose_code) {
		// Something did not go quite right.
		return 1;
	}

	return 0;
}
