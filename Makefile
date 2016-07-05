CXXFLAGS ?= -O2
CXXFLAGS += -std=c++11 -Wall -Wextra -Wpedantic

.PHONY: clean test

test: pexec
	@echo -e "\nIf you can see 'Hello World' being printed, the test is successfull\n"
	printf '#include <iostream>\n int main(){ std::cout << "Hello World\\n"; return 0; }' \
	    | $(CXX) $(CXXFLAGS) -xc++ - -o /dev/stdout | ./pexec

%: %.c
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $< -o $@

clean:
	rm pexec -vf
