################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/Team.c \
../src/utils.c 

OBJS += \
./src/Team.o \
./src/utils.o 

C_DEPS += \
./src/Team.d \
./src/utils.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I/home/utnso/workspace/tp-2020-1c-MATE-OS/SharedLibrary -I"/home/utnso/workspace/tp-2020-1c-MATE-OS/Team/src" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


