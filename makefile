# makefile

TEST_COMMANDS=./alignchar testfiles/INPUT.txt temp && \
              diff temp testfiles/INPUT_expected.txt && \
              rm temp

###############################################################################

.PHONY: test clean

alignchar: alignchar.c
	gcc alignchar.c -o alignchar -std=c99 -Wall -Wextra -Wconversion -O3

# Info on subst:
# https://www.gnu.org/software/make/manual/html_node/Text-Functions.html
test: alignchar
	$(subst INPUT,abc,      $(TEST_COMMANDS))
	$(subst INPUT,allbs,    $(TEST_COMMANDS))
	$(subst INPUT,emptyfile,$(TEST_COMMANDS))
	$(subst INPUT,long,     $(TEST_COMMANDS))
	$(subst INPUT,nestedif, $(TEST_COMMANDS))
	echo ALL TESTS PASSED

clean:
	rm -f alignchar
