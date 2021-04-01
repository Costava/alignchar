# alignchar

For the given input file, for each line that ends in `TARGET_CHAR`,
align that character to the `TARGET_COLUMN` position using `FILL_CHAR`
and output to the given path.
Lines longer than `TARGET_COLUMN` are unchanged.

Usage:  
`./alignchar path/to/inputfile path/to/outputfile`

The program is basic and narrow so for the sake of simplicity,
other parameters must be changed in the section at the top of the source file
and then the program must be recompiled.

Only depends on the C99 standard library.  
Never dynamically allocates memory.  
May produce unexpected results:
- On non-ASCII files
- On files with CRLF line endings

## Build
Build with `make` or any C99 compiler  
Run the tests with `make test`

## License
MIT License. See file `LICENSE.txt`.
