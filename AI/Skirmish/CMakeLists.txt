### Skirmish AIs
#
# Functions and macros defined in this file:
# * SkirmishAIMessage
#

add_definitions(-DBUILDING_SKIRMISH_AI)

set(SKIRMISH_SRC            "${CMAKE_CURRENT_SOURCE_DIR}")
set(SKIRMISH_AI_LIBS        "${AI_LIBS_DIR}/AI/Skirmish")
set(SKIRMISH_AI_DATA        "${AI_DATA_DIR}/AI/Skirmish")
set(SKIRMISHAI_FIND_QUIETLY ${AI_FIND_QUIETLY})


# Print a message, but only if we are allowed to speak.
macro    (SkirmishAIMessage type msg)
	if    (NOT SKIRMISHAI_FIND_QUIETLY)
		message(STATUS "${msg}")
	endif (NOT SKIRMISHAI_FIND_QUIETLY)
endmacro (SkirmishAIMessage typemsg)


# Add all Skirmish AI submodules
GetListOfSubModules(SKIRMISH_AI_DIRS)
foreach    (skirmishAIDir ${SKIRMISH_AI_DIRS})
	Add_Subdirectory(${skirmishAIDir})
endforeach (skirmishAIDir)
