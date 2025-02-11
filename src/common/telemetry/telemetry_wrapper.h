// telemetry_wrapper.h
#ifdef __cplusplus
extern "C" {
#endif

typedef void* OPTL_TRACE_HANDLE;

void init_tracer();
void cleanup_tracer();
OPTL_TRACE_HANDLE start_span(const char* name);
void end_span(OPTL_TRACE_HANDLE handle);

#ifdef __cplusplus
}
#endif