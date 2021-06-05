# makefile

TEST_COMMANDS=./alignchar -p 79 -i testfiles/INPUT.txt -o temp && \
              diff temp testfiles/INPUT_expected.txt && \
              rm temp

################################################################################

.PHONY: build test clean

alignchar: alignchar.c
	gcc alignchar.c -o alignchar -std=c99 -Wall -Wextra -Wconversion -O3

build: alignchar

# Info on subst:
# https://www.gnu.org/software/make/manual/html_node/Text-Functions.html
test: alignchar
	$(subst INPUT,abc,      $(TEST_COMMANDS))
	$(subst INPUT,allbs,    $(TEST_COMMANDS))
	$(subst INPUT,emptyfile,$(TEST_COMMANDS))
	$(subst INPUT,long,     $(TEST_COMMANDS))
	$(subst INPUT,nestedif, $(TEST_COMMANDS))
	# Ensure returns 0
	./alignchar --help
	# Ensure returns 0
	./alignchar --version
	# Check failure
	! ./alignchar
	! ./alignchar --this-arg-does-not-exist
	! ./alignchar -i NONEXISTENT_FILE_IN___ -o NONEXISTENT_FILE_OUT___
	# Test --in-place
	cp testfiles/inplace.txt inplace_copy.txt
	./alignchar -i inplace_copy.txt --in-place
	diff inplace_copy.txt testfiles/inplace_expected.txt
	rm inplace_copy.txt
	# Test c, p, and f args
	./alignchar -i testfiles/cpf.txt -o temp -c ] -p 10 -f F
	diff temp testfiles/cpf_expected.txt
	rm temp
	# Test t arg
	./alignchar -i testfiles/t.txt -o temp -t 2 -f + -p 6
	diff temp testfiles/t_expected.txt
	rm temp
	# All done
	echo ALL TESTS PASSED

clean:
	rm -f alignchar
