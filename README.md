# alignchar

```
$ ./alignchar --help
alignchar: For the given input file, for each line that ends in the target
           character (Default: '\'), align the target character to the
           target column position (Default: 80. First column is 1) using the
           fill character (Default: ' '). Non-matching lines, lines longer
           than 2047, and lines where the target character falls on or
           after the target column position are unchanged.

Usage examples:
  alignchar [options] -i <input file> -o <output file>
  alignchar [options] -i <input file> --in-place

An input file must be specified (-i or --input).
Either an output file must be specified (-o or --output)
    or --in-place must be specified meaning modify the input file.
Options may be given in any order.

Options:
  --help               Stop parsing options, print help, exit(0)
  --version            Stop parsing options, print version, exit(0)

  -i, --input <path>   Specify input file (required)
  -o, --output <path>  Specify output file
                       (mutually exclusive with --in-place)
                       Do NOT specify the same path as for input
                       (use --in-place instead)
  --in-place           Modify input file
                       (mutually exclusive with -o, --output)
  -c, --char <char>    Specify the character to be aligned (Default: '\')
  -p, --position <n>   Specify the column to align the character to
                       (Default: 80)
  -f, --fill <char>    Specify the fill character (Default: ' ')
  -t, --tab-width <n>  Specify tab width (Default: 4)

```

Only depends on the C99 standard library.  
Never dynamically allocates memory.  
May produce unexpected results:
- On non-ASCII files
- On files with CRLF line endings

## Build
Build with `make` or any C99 compiler  
Run the tests with `make test`

## License
BSD 2-Clause License. See file `LICENSE.txt`.
