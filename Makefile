TARGET   = freq
CFLAGS   = -std=c11 -g -static

SRCDIR   = src
OBJDIR   = obj
BINDIR   = bin

SOURCES  := $(wildcard $(SRCDIR)/*.c)
INCLUDES := $(wildcard $(SRCDIR)/*.h)
OBJECTS  := $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
DEPENDS  := $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.d)

$(BINDIR)/$(TARGET): $(OBJECTS)
	mkdir -p $(BINDIR)
	$(CC) -o $@ $(OBJECTS) $(LDFLAGS)

-include $(DEPENDS)

$(OBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.c
	mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c -MMD -MP $< -o $@

test: $(BINDIR)/$(TARGET)
	./test.sh

clean:
	rm -rf $(BINDIR) $(OBJDIR) *~ tmp*

.PHONY: test clean
