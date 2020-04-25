source = MP0
all: $(source)
$(source):
%: %.c
	 gcc $< -o char_count
%: %.cpp
	 g++ $< -o char_count
clean:
	 rm -rf char_count input.txt
