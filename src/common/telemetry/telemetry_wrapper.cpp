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
#include <opentelemetry/nostd/shared_ptr.h>
#include <opentelemetry/trace/provider.h>

namespace trace_api = opentelemetry::trace;
namespace trace_sdk = opentelemetry::sdk::trace;
namespace trace_exporter = opentelemetry::exporter::trace;
namespace otlp = opentelemetry::exporter::otlp;
namespace nostd = opentelemetry::nostd;

std::shared_ptr<opentelemetry::sdk::trace::TracerProvider> provider;

struct SpanAndScope {
    nostd::shared_ptr<trace_api::Span> span;
    nostd::shared_ptr<trace_api::Scope> scope;
};

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
    nostd::shared_ptr<trace_api::TracerProvider> noop;
    trace_api::Provider::SetTracerProvider(noop);
}

OPTL_TRACE_HANDLE start_span(const char* name) {
    auto tracer = trace_api::Provider::GetTracerProvider()->GetTracer("osconfig_tracer");
    auto span = tracer->StartSpan(name);
    auto scope = std::make_shared<trace_api::Scope>(tracer->WithActiveSpan(span));
    span->AddEvent("StartSpan");
    span->SetStatus(trace_api::StatusCode::kUnset, "Span started");
    return new SpanAndScope{span, scope};
}

void end_span(OPTL_TRACE_HANDLE handle) {
    auto span_and_scope = static_cast<SpanAndScope*>(handle);
    if (span_and_scope && span_and_scope->span) {
        span_and_scope->span->SetStatus(trace_api::StatusCode::kOk, "Span ended");
        span_and_scope->span->End();
        delete span_and_scope;
    }
}