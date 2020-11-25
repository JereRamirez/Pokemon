################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../delibird/comms/messages.c \
../delibird/comms/pokeio.c \
../delibird/comms/serialization.c 

OBJS += \
./delibird/comms/messages.o \
./delibird/comms/pokeio.o \
./delibird/comms/serialization.o 

C_DEPS += \
./delibird/comms/messages.d \
./delibird/comms/pokeio.d \
./delibird/comms/serialization.d 


# Each subdirectory must supply rules for building sources it contributes
delibird/comms/%.o: ../delibird/comms/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


