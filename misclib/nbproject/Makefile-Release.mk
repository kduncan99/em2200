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
	${OBJECTDIR}/DataHandler.o \
	${OBJECTDIR}/GeneralRegister.o \
	${OBJECTDIR}/InstructionWord.o \
	${OBJECTDIR}/SimpleFile.o \
	${OBJECTDIR}/SuperString.o \
	${OBJECTDIR}/SystemLog.o \
	${OBJECTDIR}/SystemTime.o \
	${OBJECTDIR}/TDate.o \
	${OBJECTDIR}/Word36.o \
	${OBJECTDIR}/Worker.o \
	${OBJECTDIR}/misclib.o


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
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libmisclib.a

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libmisclib.a: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libmisclib.a
	${AR} -rv ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libmisclib.a ${OBJECTFILES} 
	$(RANLIB) ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libmisclib.a

${OBJECTDIR}/DataHandler.o: DataHandler.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/DataHandler.o DataHandler.cpp

${OBJECTDIR}/GeneralRegister.o: GeneralRegister.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/GeneralRegister.o GeneralRegister.cpp

${OBJECTDIR}/InstructionWord.o: InstructionWord.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/InstructionWord.o InstructionWord.cpp

${OBJECTDIR}/SimpleFile.o: SimpleFile.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/SimpleFile.o SimpleFile.cpp

${OBJECTDIR}/SuperString.o: SuperString.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/SuperString.o SuperString.cpp

${OBJECTDIR}/SystemLog.o: SystemLog.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/SystemLog.o SystemLog.cpp

${OBJECTDIR}/SystemTime.o: SystemTime.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/SystemTime.o SystemTime.cpp

${OBJECTDIR}/TDate.o: TDate.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/TDate.o TDate.cpp

${OBJECTDIR}/Word36.o: Word36.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Word36.o Word36.cpp

${OBJECTDIR}/Worker.o: Worker.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Worker.o Worker.cpp

${OBJECTDIR}/misclib.o: misclib.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/misclib.o misclib.cpp

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libmisclib.a

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
