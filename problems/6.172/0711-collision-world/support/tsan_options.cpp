// Stops at the first data race, so the report is attributed to the test that was running.
#if defined(__has_feature)
#if __has_feature(thread_sanitizer)
extern "C" const char* __tsan_default_options() { return "halt_on_error=1"; }
#endif
#endif
