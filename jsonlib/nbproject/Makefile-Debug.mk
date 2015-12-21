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
CND_PLATFORM=GNU-Linux
CND_DLIB_EXT=so
CND_CONF=Debug
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/JSONArrayValue.o \
	${OBJECTDIR}/JSONBooleanValue.o \
	${OBJECTDIR}/JSONNullValue.o \
	${OBJECTDIR}/JSONNumberValue.o \
	${OBJECTDIR}/JSONObjectValue.o \
	${OBJECTDIR}/JSONParser.o \
	${OBJECTDIR}/JSONStringValue.o \
	${OBJECTDIR}/JSONValue.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libjsonlib.a

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libjsonlib.a: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libjsonlib.a
	${AR} -rv ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libjsonlib.a ${OBJECTFILES} 
	$(RANLIB) ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libjsonlib.a

${OBJECTDIR}/JSONArrayValue.o: nbproject/Makefile-${CND_CONF}.mk JSONArrayValue.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -Wall -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/JSONArrayValue.o JSONArrayValue.cpp

${OBJECTDIR}/JSONBooleanValue.o: nbproject/Makefile-${CND_CONF}.mk JSONBooleanValue.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -Wall -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/JSONBooleanValue.o JSONBooleanValue.cpp

${OBJECTDIR}/JSONNullValue.o: nbproject/Makefile-${CND_CONF}.mk JSONNullValue.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -Wall -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/JSONNullValue.o JSONNullValue.cpp

${OBJECTDIR}/JSONNumberValue.o: nbproject/Makefile-${CND_CONF}.mk JSONNumberValue.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -Wall -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/JSONNumberValue.o JSONNumberValue.cpp

${OBJECTDIR}/JSONObjectValue.o: nbproject/Makefile-${CND_CONF}.mk JSONObjectValue.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -Wall -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/JSONObjectValue.o JSONObjectValue.cpp

${OBJECTDIR}/JSONParser.o: nbproject/Makefile-${CND_CONF}.mk JSONParser.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -Wall -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/JSONParser.o JSONParser.cpp

${OBJECTDIR}/JSONStringValue.o: nbproject/Makefile-${CND_CONF}.mk JSONStringValue.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -Wall -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/JSONStringValue.o JSONStringValue.cpp

${OBJECTDIR}/JSONValue.o: nbproject/Makefile-${CND_CONF}.mk JSONValue.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -Wall -std=c++11 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/JSONValue.o JSONValue.cpp

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libjsonlib.a

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
