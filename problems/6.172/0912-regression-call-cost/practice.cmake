# Instrument only workload.cpp; profiler.cpp and workload_plain.cpp stay uninstrumented.
set_source_files_properties("${exercise}/support/workload.cpp" PROPERTIES
  COMPILE_OPTIONS "-finstrument-functions")
