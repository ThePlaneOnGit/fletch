override CFLAGS+=-Wall -Wextra -Werror
TARGET=fletch

.PHONY: all
all: $(TARGET)

$(TARGET): $(wildcard src/*.c)
	gcc -o $@ $(CFLAGS) $(CPPFLAGS) $< $(LDLIBS)

.PHONY: install
install: /bin/$(TARGET)

/bin/$(TARGET): $(TARGET)
	$(shell [ -f $@ ] && rm -rf $@)
# make a new symlink for efetch
	@echo "Making new symlink for /bin/efetch"
	ln $< $@
	@echo installed
