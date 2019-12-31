###############################################################################
# Boiler-plate

OBJECTS = $(addsuffix .o, $(basename $(SRC)))
SYS_OBJECTS = $(addsuffix .o, $(basename $(SYS_SRC)))

# cross-platform directory manipulation
ifeq ($(shell echo $$OS),$$OS)
    MAKEDIR = if not exist "$(1)" mkdir "$(1)"
    RM = rmdir /S /Q "$(1)"
else
    MAKEDIR = '$(SHELL)' -c "mkdir -p \"$(1)\""
    RM = '$(SHELL)' -c "rm -rf \"$(1)\""
endif

# OBJDIR := BUILD
# # Move to the build directory
# ifeq (,$(filter $(OBJDIR),$(notdir $(CURDIR))))
# .SUFFIXES:
# mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
# MAKETARGET = '+$(MAKE)' --no-print-directory -C $(OBJDIR) -f '$(mkfile_path)' \
# 		'SRCDIR=$(CURDIR)' $(MAKECMDGOALS)
# .PHONY: $(OBJDIR) clean
# all:
# 	+@$(call MAKEDIR,$(OBJDIR))
# 	+@$(MAKETARGET)
# $(OBJDIR): all
# Makefile : ;
# % :: $(OBJDIR) ; :
# clean :
# 	$(call RM,$(OBJDIR))

# else


# trick rules into thinking we are in the root, when we are in the bulid dir
# VPATH = ..

# Boiler-plate
###############################################################################
# Rules

.PHONY: all lst size


all: $(PROJECT).bin $(PROJECT).hex size


.s.o:
	+@$(call MAKEDIR,$(dir $@))
	+@echo "Assemble: $(notdir $<)"
	$(Q)$(AS) -c $(ASM_FLAGS) -o $@ $<


.S.o:
	+@$(call MAKEDIR,$(dir $@))
	+@echo "Assemble: $(notdir $<)"
	$(Q)$(AS) -c $(ASM_FLAGS) -o $@ $<

.c.o:
	+@$(call MAKEDIR,$(dir $@))
	+@echo "Compile: $(notdir $<)"
	$(Q)$(CC) $(C_FLAGS) $(INCLUDE_PATHS) -o $@ $<

.cpp.o:
	+@$(call MAKEDIR,$(dir $@))
	+@echo "Compile: $(notdir $<)"
	$(Q)$(CPP) $(CXX_FLAGS) $(INCLUDE_PATHS) -o $@ $<


$(PROJECT).link_script.ld: $(LINKER_SCRIPT)
	echo WAT $@ - $^
	$(Q)$(PREPROC) $< -o $@



$(PROJECT).elf: $(OBJECTS) $(SYS_OBJECTS) $(PROJECT).link_script.ld 
	+@echo "$(filter %.o, $^)" > .link_options.txt
	+@echo "link: $(notdir $@)"
	$(Q)$(LD) $(LD_FLAGS) -T $(filter-out %.o, $^) $(LIBRARY_PATHS) --output $@ @.link_options.txt $(LIBRARIES) $(LD_SYS_LIBS)


$(PROJECT).bin: $(PROJECT).elf
	$(ELF2BIN) -O binary $< $@
	+@echo "===== bin file ready to flash: $@ =====" 

$(PROJECT).hex: $(PROJECT).elf
	$(ELF2BIN) -O ihex $< $@

flash: $(PROJECT).bin
	cp $(PROJECT).bin $(MOUNT_POINT)

serial:
	stty -F /dev/ttyACM0 speed 9600 raw
	cat /dev/ttyACM0

clean:
	rm -f $(OBJECTS) $(SYS_OBJECTS) $(DEPS) $(PROJECT).elf $(PROJECT).bin $(PROJECT).hex $(PROJECT).link_script.ld

# Rules
###############################################################################
# Dependencies

DEPS = $(OBJECTS:.o=.d) $(SYS_OBJECTS:.o=.d)
-include $(DEPS)
# endif

# Dependencies
###############################################################################
# Catch-all

%: ;

# Catch-all
###############################################################################
