file(GLOB_RECURSE SMDATA_MIXPLAY_FILES_SRC "MixPlay*.cpp")
file(GLOB_RECURSE SMDATA_MIXPLAY_FILES_HPP "MixPlay*.h")

source_group("MixPlay"
             FILES
             ${SMDATA_MIXPLAY_FILES_SRC}
             ${SMDATA_MIXPLAY_FILES_HPP})
