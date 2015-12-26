##################################
# Makefile helper.               #
# Used as Kdevelop root makefile #
##################################

CPPCHECK = cppcheck
CPPCHECK_ARGS = --std=c++11 --template=gcc --enable=all --inconclusive --std=posix engine/src/ -I engine/src/ -I external/.build/linux64/include -I external/.build/linux64/include -UCETECH_TEST -j 4 -i external/.build/linux64/include/catch -i engine/src/tests/

linux64-clang:
	python ./make.py
	build/linux64_clang/bin/cetech_testDebug --order rand -b

clean:
	python ./make.py clean

#########
# Utils #
#########
.PHONY: tidy
tidy:
	@echo Tidy...
	clang-tidy -header-filter='.*' `find ./engine/src/ -regex ".*\.\(c\|cc\|h\)"`  -- -Iengine/src/ -Iexternal/.build/linux64/include -DCETECH_LINUX -DCETECH_SDL2 -DDEBUG -DCETECH_DEVELOP
	
.PHONY: cppcheck
cppcheck:
	@echo Analyze...
	$(CPPCHECK) $(CPPCHECK_ARGS)

.PHONY: cppcheck-html
cppcheck-html:
	$(CPPCHECK) $(CPPCHECK_ARGS) --xml 2> cppcheck.xml
	cppcheck-htmlreport --report-dir=./build/cppcheck --file cppcheck.xml
	rm cppcheck.xml
	
.PHONY: scan-build64
scan-build64: clean
	scan-build -V -analyze-headers make linux64-clang

.PHONY: uncrustify
uncrustify:
	@echo Uncrustify...
	-@find ./engine/src/ -regex ".*\.\(c\|cc\|h\)" -print0 | xargs -0 uncrustify -l c -c ./scripts/uncrustify.cfg --no-backup
#########
