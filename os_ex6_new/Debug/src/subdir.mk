################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
O_SRCS += \
../src/chardev.mod.o \
../src/chardev.o 

C_SRCS += \
../src/chardev.c \
../src/chardev.mod.c 

OBJS += \
./src/chardev.o \
./src/chardev.mod.o 

C_DEPS += \
./src/chardev.d \
./src/chardev.mod.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


