TARGET = kernel.elf
OBJS = main.o color.o screen.o point2d.o
DEPENDS = $(join $(dir $(OBJS)),$(addprefix .,$(notdir $(OBJS:.o=.d))))

LIBS_PATH = $(HOME)/osbook/devenv/x86_64-elf
EDK2DIR = $(HOME)/edk2

CPPFLAGS += -I. -I$(LIBS_PATH)/include/c++/v1 -I$(LIBS_PATH)/include -I$(LIBS_PATH)/include/freetype2 \
            -I$(EDK2DIR)/MdePkg/Include -I$(EDK2DIR)/MdePkg/Include/X64 \
            -nostdlibinc -D__ELF__ -D_LDBL_EQ_DBL -D_GNU_SOURCE -D_POSIX_TIMERS \
            -DEFIAPI='__attribute__((ms_abi))'
CFLAGS   += -O2 -Wall -g --target=x86_64-elf -ffreestanding -mno-red-zone
CXXFLAGS += -O2 -Wall -g --target=x86_64-elf -ffreestanding -mno-red-zone \
            -fno-exceptions -fno-rtti -std=c++17
LDFLAGS  += --entry KernelMain -z norelro --image-base 0x100000 --static -z separate-code -L$(LIBS_PATH)/lib

.PHONY: all clean
all: $(TARGET)

clean:
	rm -rf *.o

kernel.elf: $(OBJS)
	ld.lld $(LDFLAGS) -o kernel.elf $(OBJS) -lc -lc++ -lc++abi -lm -lfreetype

%.o: %.cpp Makefile
	clang++ $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

.%.d: %.cpp
	clang++ $(CPPFLAGS) $(CXXFLAGS) -MM $< > $@
	$(eval OBJ = $(<:.cpp=.o))
	sed -I '' -e 's|$(notdir $(OBJ))|$(OBJ)|' $@

%.o: %.c Makefile
	clang $(CPPFLAGS) $(CFLAGS) -c $< -o $@

.%.d: %.c
	clang $(CPPFLAGS) $(CFLAGS) -MM $< > $@
	$(eval OBJ = $(<:.c=.o))
	sed -I '' -e 's|$(notdir $(OBJ))|$(OBJ)|' $@

.PHONY: depends
depends:
	$(MAKE) $(DEPENDS)

-include $(DEPENDS)