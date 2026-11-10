# Instrument only the workload: the hooks in profiler.cpp must stay uninstrumented.
set_source_files_properties("${exercise}/support/workload.cpp" PROPERTIES
  COMPILE_OPTIONS "-finstrument-functions")
