// telemetry_wrapper.cpp
#include "telemetry_wrapper.h"
#include <opentelemetry/exporters/ostream/span_exporter.h>
#include <opentelemetry/exporters/otlp/otlp_http_exporter_factory.h>
#include <opentelemetry/exporters/otlp/otlp_http_exporter_options.h>
#include <opentelemetry/sdk/trace/tracer_provider.h>
#include <opentelemetry/sdk/trace/exporter.h>
#include <opentelemetry/sdk/trace/processor.h>
#include <opentelemetry/sdk/trace/simple_processor_factory.h>
#include <opentelemetry/sdk/trace/tracer_provider_factory.h>
#include "opentelemetry/sdk/trace/batch_span_processor_factory.h"
#include "opentelemetry/sdk/trace/batch_span_processor_options.h"
#include <opentelemetry/trace/provider.h>

namespace trace_api = opentelemetry::trace;
namespace trace_sdk = opentelemetry::sdk::trace;
namespace trace_exporter = opentelemetry::exporter::trace;
namespace otlp = opentelemetry::exporter::otlp;

std::shared_ptr<opentelemetry::sdk::trace::TracerProvider> provider;

void init_tracer() {
    // trace_sdk::BatchSpanProcessorOptions bspOpts{};
    otlp::OtlpHttpExporterOptions opts;
    opts.url = "http://localhost:4318/v1/traces";

    auto exporter  = otlp::OtlpHttpExporterFactory::Create(opts);
    auto processor = trace_sdk::SimpleSpanProcessorFactory::Create(std::move(exporter));
    // auto exporter = std::unique_ptr<trace_sdk::SpanExporter>(new trace_exporter::OStreamSpanExporter); // immediately forwards ended spans to the exporter
    // auto processor = trace_sdk::SimpleSpanProcessorFactory::Create(std::move(exporter));
    
    provider = trace_sdk::TracerProviderFactory::Create(std::move(processor));
    const std::shared_ptr<trace_api::TracerProvider> &api_provider = provider;

    // auto provider = nostd::shared_ptr<trace_sdk::TracerProvider>(new trace_sdk::TracerProvider(std::move(processor)));
    // set the global trace provider
    trace_api::Provider::SetTracerProvider(api_provider);
    // tracer = provider->GetTracer("osconfig_tracer");
}

void cleanup_tracer() {
    // trace::Provider::SetTracerProvider(std::shared_ptr<sdktrace::TracerProvider>(nullptr));
    provider->ForceFlush();
    provider.reset();
    std::shared_ptr<trace_api::TracerProvider> noop;
    trace_api::Provider::SetTracerProvider(noop);
}

OPTL_TRACE_HANDLE start_span(const char* name) {
    auto tracer = trace_api::Provider::GetTracerProvider()->GetTracer("osconfig_tracer");
    auto span = tracer->StartSpan(name);
    // auto scope = tracer->WithActiveSpan(span);
    span->AddEvent("StartSpan");
    span->SetStatus(trace_api::StatusCode::kUnset, "Span started");
    return new std::shared_ptr<trace_api::Span>(span.get()); 
}

void end_span(OPTL_TRACE_HANDLE handle) {
    auto span_ptr = static_cast<std::shared_ptr<trace_api::Span>*>(handle);
    if (span_ptr && *span_ptr) {
        (*span_ptr)->SetStatus(trace_api::StatusCode::kOk, "Span ended");
        (*span_ptr)->End();
        delete span_ptr;
    }
}