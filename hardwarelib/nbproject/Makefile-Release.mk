#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
GREP=grep
NM=nm
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=g++
CXX=g++
FC=gfortran
AS=as

# Macros
CND_PLATFORM=GNU-Linux-x86
CND_DLIB_EXT=so
CND_CONF=Release
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/ChannelModule.o \
	${OBJECTDIR}/Controller.o \
	${OBJECTDIR}/Device.o \
	${OBJECTDIR}/DiskController.o \
	${OBJECTDIR}/DiskDevice.o \
	${OBJECTDIR}/FileSystemDiskDevice.o \
	${OBJECTDIR}/IOProcessor.o \
	${OBJECTDIR}/IoAccessControlList.o \
	${OBJECTDIR}/Node.o \
	${OBJECTDIR}/Processor.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=-m64
CXXFLAGS=-m64

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libhardwarelib.a

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libhardwarelib.a: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libhardwarelib.a
	${AR} -rv ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libhardwarelib.a ${OBJECTFILES} 
	$(RANLIB) ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libhardwarelib.a

${OBJECTDIR}/ChannelModule.o: ChannelModule.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/ChannelModule.o ChannelModule.cpp

${OBJECTDIR}/Controller.o: Controller.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Controller.o Controller.cpp

${OBJECTDIR}/Device.o: Device.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Device.o Device.cpp

${OBJECTDIR}/DiskController.o: DiskController.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/DiskController.o DiskController.cpp

${OBJECTDIR}/DiskDevice.o: DiskDevice.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/DiskDevice.o DiskDevice.cpp

${OBJECTDIR}/FileSystemDiskDevice.o: FileSystemDiskDevice.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/FileSystemDiskDevice.o FileSystemDiskDevice.cpp

${OBJECTDIR}/IOProcessor.o: IOProcessor.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/IOProcessor.o IOProcessor.cpp

${OBJECTDIR}/IoAccessControlList.o: IoAccessControlList.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/IoAccessControlList.o IoAccessControlList.cpp

${OBJECTDIR}/Node.o: Node.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Node.o Node.cpp

${OBJECTDIR}/Processor.o: Processor.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Processor.o Processor.cpp

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libhardwarelib.a

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
