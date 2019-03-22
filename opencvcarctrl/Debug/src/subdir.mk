################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../src/main.cc 

CC_DEPS += \
./src/main.d 

OBJS += \
./src/main.o 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v7 Linux g++ compiler'
	arm-linux-gnueabihf-g++ -Wall -O0 -g3 -I"/home/mirel/.opt/SDK/2017.4/data/embeddedsw/ThirdParty/opencv/aarch32/include" -I"/home/mirel/Zybo/SDCar/build/tmp/sysroots/plnx_arm/usr/include" -I"/home/mirel/.opt/petalinux/tools/linux-i386/gcc-arm-linux-gnueabi/arm-linux-gnueabihf/include" -I"/home/mirel/.opt/petalinux/tools/linux-i386/gcc-arm-linux-gnueabi/arm-linux-gnueabihf/include/c++/6.2.1" -I"/home/mirel/.opt/petalinux/tools/linux-i386/gcc-arm-linux-gnueabi/arm-linux-gnueabihf/include/c++/6.2.1/arm-linux-gnueabihf" -I"/home/mirel/.opt/petalinux/tools/linux-i386/gcc-arm-linux-gnueabi/arm-linux-gnueabihf/include/c++/6.2.1/backward" -I"/home/mirel/.opt/petalinux/tools/linux-i386/gcc-arm-linux-gnueabi/arm-linux-gnueabihf/libc/usr/include" -I"/home/mirel/.opt/petalinux/tools/linux-i386/gcc-arm-linux-gnueabi/lib/gcc/arm-linux-gnueabihf/6.2.1/include" -I"/home/mirel/.opt/petalinux/tools/linux-i386/gcc-arm-linux-gnueabi/lib/gcc/arm-linux-gnueabihf/6.2.1/include-fixed" -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


