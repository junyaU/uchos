TARGET = kernel.elf
OBJS = main.o point2d.o newlib_support.o libcxx_support.o asm_utils.o
DEPENDS = $(join $(dir $(OBJS)),$(addprefix .,$(notdir $(OBJS:.o=.d))))

include common.mk
include ./memory/Makefile
include ./graphics/Makefile
include ./interrupt/Makefile
include ./timers/Makefile

.PHONY: all 
all: $(TARGET)

.PHONY: clean
clean:
	rm -f *.o *.d

kernel.elf: $(OBJS)
	ld.lld $(LDFLAGS) -o kernel.elf $(OBJS) -lc -lc++ -lc++abi -lm -lfreetype

.PHONY: depends
depends:
	$(MAKE) $(DEPENDS)

-include $(DEPENDS)