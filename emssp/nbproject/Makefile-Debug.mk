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
CND_CONF=Debug
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include emssp-Makefile.mk

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/PersistableNodeTable.o \
	${OBJECTDIR}/SSPConsole.o \
	${OBJECTDIR}/SSPPanel.o \
	${OBJECTDIR}/SubSystem.o \
	${OBJECTDIR}/main.o


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
LDLIBSOPTIONS=../execlib/dist/Debug/GNU-Linux-x86/libexeclib.a ../hardwarelib/dist/Debug/GNU-Linux-x86/libhardwarelib.a ../misclib/dist/Debug/GNU-Linux-x86/libmisclib.a ../jsonlib/dist/Debug/GNU-Linux-x86/libjsonlib.a -lpthread

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/emssp

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/emssp: ../execlib/dist/Debug/GNU-Linux-x86/libexeclib.a

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/emssp: ../hardwarelib/dist/Debug/GNU-Linux-x86/libhardwarelib.a

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/emssp: ../misclib/dist/Debug/GNU-Linux-x86/libmisclib.a

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/emssp: ../jsonlib/dist/Debug/GNU-Linux-x86/libjsonlib.a

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/emssp: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.cc} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/emssp ${OBJECTFILES} ${LDLIBSOPTIONS}

${OBJECTDIR}/PersistableNodeTable.o: nbproject/Makefile-${CND_CONF}.mk PersistableNodeTable.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -Wall -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/PersistableNodeTable.o PersistableNodeTable.cpp

${OBJECTDIR}/SSPConsole.o: nbproject/Makefile-${CND_CONF}.mk SSPConsole.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -Wall -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/SSPConsole.o SSPConsole.cpp

${OBJECTDIR}/SSPPanel.o: nbproject/Makefile-${CND_CONF}.mk SSPPanel.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -Wall -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/SSPPanel.o SSPPanel.cpp

${OBJECTDIR}/SubSystem.o: nbproject/Makefile-${CND_CONF}.mk SubSystem.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -Wall -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/SubSystem.o SubSystem.cpp

${OBJECTDIR}/main.o: nbproject/Makefile-${CND_CONF}.mk main.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -Wall -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/main.o main.cpp

# Subprojects
.build-subprojects:
	cd ../execlib && ${MAKE}  -f Makefile CONF=Debug
	cd ../hardwarelib && ${MAKE}  -f Makefile CONF=Debug
	cd ../misclib && ${MAKE}  -f Makefile CONF=Debug
	cd ../jsonlib && ${MAKE}  -f Makefile CONF=Debug

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/emssp

# Subprojects
.clean-subprojects:
	cd ../execlib && ${MAKE}  -f Makefile CONF=Debug clean
	cd ../hardwarelib && ${MAKE}  -f Makefile CONF=Debug clean
	cd ../misclib && ${MAKE}  -f Makefile CONF=Debug clean
	cd ../jsonlib && ${MAKE}  -f Makefile CONF=Debug clean

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
