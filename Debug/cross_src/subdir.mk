################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../cross_src/CrossMatch.cpp \
../cross_src/CrossMatchSphere.cpp \
../cross_src/Partition.cpp \
../cross_src/PartitionSphere.cpp \
../cross_src/RedisTest.cpp \
../cross_src/StarFile.cpp \
../cross_src/StarFileFits.cpp \
../cross_src/cmutils.cpp \
../cross_src/main.cpp 

OBJS += \
./cross_src/CrossMatch.o \
./cross_src/CrossMatchSphere.o \
./cross_src/Partition.o \
./cross_src/PartitionSphere.o \
./cross_src/RedisTest.o \
./cross_src/StarFile.o \
./cross_src/StarFileFits.o \
./cross_src/cmutils.o \
./cross_src/main.o 

CPP_DEPS += \
./cross_src/CrossMatch.d \
./cross_src/CrossMatchSphere.d \
./cross_src/Partition.d \
./cross_src/PartitionSphere.d \
./cross_src/RedisTest.d \
./cross_src/StarFile.d \
./cross_src/StarFileFits.d \
./cross_src/cmutils.d \
./cross_src/main.d 


# Each subdirectory must supply rules for building sources it contributes
cross_src/%.o: ../cross_src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -DLINUX2 -D_POSIX_PTHREAD_SEMANTICS -I/usr/lib/jvm/jdk1.8.0_101/include -I"/home/wamdm/workspace/Squirrel/library/acl/lib_acl_cpp/include" -I/usr/lib/jvm/jdk1.8.0_101/include/linux -I"/home/wamdm/workspace/Squirrel/library/astrometry.net-0.38/include" -I"/home/wamdm/workspace/Squirrel/library/cfitsio/include" -I"/home/wamdm/workspace/Squirrel/library/wcstools-3.8.5/libwcs" -O3 -g3 -Wall -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


