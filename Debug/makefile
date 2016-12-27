################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include cross_src/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(CC_DEPS)),)
-include $(CC_DEPS)
endif
ifneq ($(strip $(C++_DEPS)),)
-include $(C++_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
ifneq ($(strip $(CXX_DEPS)),)
-include $(CXX_DEPS)
endif
ifneq ($(strip $(CPP_DEPS)),)
-include $(CPP_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: Squirrel

# Tool invocations
Squirrel: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C++ Linker'
	g++ -L/usr/lib/jvm/jdk1.8.0_101/jre/lib/amd64/server -L"/home/wamdm/workspace/Squirrel/library/acl/lib_protocol/lib" -L"/home/wamdm/workspace/Squirrel/library/acl/lib_acl/lib" -L"/home/wamdm/workspace/Squirrel/library/acl/lib_acl_cpp/lib" -L/usr/bin/java -L"/home/wamdm/workspace/Squirrel/library/astrometry.net-0.38/lib" -L"/home/wamdm/workspace/Squirrel/library/wcstools-3.8.5/libwcs" -L"/home/wamdm/workspace/Squirrel/library/cfitsio/lib" -fPIC -o "Squirrel" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(CC_DEPS)$(C++_DEPS)$(EXECUTABLES)$(C_UPPER_DEPS)$(CXX_DEPS)$(OBJS)$(CPP_DEPS)$(C_DEPS) Squirrel
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
