/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "compiler_options.h"

#include <fstream>

namespace art {

CompilerOptions::CompilerOptions()
    : compiler_filter_(CompilerFilter::kDefaultCompilerFilter),
      huge_method_threshold_(kDefaultHugeMethodThreshold),
      large_method_threshold_(kDefaultLargeMethodThreshold),
      small_method_threshold_(kDefaultSmallMethodThreshold),
      tiny_method_threshold_(kDefaultTinyMethodThreshold),
      num_dex_methods_threshold_(kDefaultNumDexMethodsThreshold),
      inline_max_code_units_(kUnsetInlineMaxCodeUnits),
      no_inline_from_(nullptr),
      boot_image_(false),
      app_image_(false),
      top_k_profile_threshold_(kDefaultTopKProfileThreshold),
      debuggable_(false),
      generate_debug_info_(kDefaultGenerateDebugInfo),
      generate_mini_debug_info_(kDefaultGenerateMiniDebugInfo),
      generate_build_id_(false),
      implicit_null_checks_(true),
      implicit_so_checks_(true),
      implicit_suspend_checks_(false),
      compile_pic_(false),
      verbose_methods_(),
      abort_on_hard_verifier_failure_(false),
      init_failure_output_(nullptr),
      dump_cfg_file_name_(""),
      dump_cfg_append_(false),
      force_determinism_(false),
      register_allocation_strategy_(RegisterAllocator::kRegisterAllocatorDefault),
      passes_to_run_(nullptr) {
}

CompilerOptions::~CompilerOptions() {
  // The destructor looks empty but it destroys a PassManagerOptions object. We keep it here
  // because we don't want to include the PassManagerOptions definition from the header file.
}

void CompilerOptions::ParseHugeMethodMax(const StringPiece& option, UsageFn Usage) {
  ParseUintOption(option, "--huge-method-max", &huge_method_threshold_, Usage);
}

void CompilerOptions::ParseLargeMethodMax(const StringPiece& option, UsageFn Usage) {
  ParseUintOption(option, "--large-method-max", &large_method_threshold_, Usage);
}

void CompilerOptions::ParseSmallMethodMax(const StringPiece& option, UsageFn Usage) {
  ParseUintOption(option, "--small-method-max", &small_method_threshold_, Usage);
}

void CompilerOptions::ParseTinyMethodMax(const StringPiece& option, UsageFn Usage) {
  ParseUintOption(option, "--tiny-method-max", &tiny_method_threshold_, Usage);
}

void CompilerOptions::ParseNumDexMethods(const StringPiece& option, UsageFn Usage) {
  ParseUintOption(option, "--num-dex-methods", &num_dex_methods_threshold_, Usage);
}

void CompilerOptions::ParseInlineMaxCodeUnits(const StringPiece& option, UsageFn Usage) {
  ParseUintOption(option, "--inline-max-code-units", &inline_max_code_units_, Usage);
}

void CompilerOptions::ParseDumpInitFailures(const StringPiece& option,
                                            UsageFn Usage ATTRIBUTE_UNUSED) {
  DCHECK(option.starts_with("--dump-init-failures="));
  std::string file_name = option.substr(strlen("--dump-init-failures=")).data();
  init_failure_output_.reset(new std::ofstream(file_name));
  if (init_failure_output_.get() == nullptr) {
    LOG(ERROR) << "Failed to allocate ofstream";
  } else if (init_failure_output_->fail()) {
    LOG(ERROR) << "Failed to open " << file_name << " for writing the initialization "
               << "failures.";
    init_failure_output_.reset();
  }
}

void CompilerOptions::ParseRegisterAllocationStrategy(const StringPiece& option,
                                                      UsageFn Usage) {
  DCHECK(option.starts_with("--register-allocation-strategy="));
  StringPiece choice = option.substr(strlen("--register-allocation-strategy=")).data();
  if (choice == "linear-scan") {
    register_allocation_strategy_ = RegisterAllocator::Strategy::kRegisterAllocatorLinearScan;
  } else if (choice == "graph-color") {
    register_allocation_strategy_ = RegisterAllocator::Strategy::kRegisterAllocatorGraphColor;
  } else {
    Usage("Unrecognized register allocation strategy. Try linear-scan, or graph-color.");
  }
}

bool CompilerOptions::ParseCompilerOption(const StringPiece& option, UsageFn Usage) {
  if (option.starts_with("--compiler-filter=")) {
    const char* compiler_filter_string = option.substr(strlen("--compiler-filter=")).data();
    if (!CompilerFilter::ParseCompilerFilter(compiler_filter_string, &compiler_filter_)) {
      Usage("Unknown --compiler-filter value %s", compiler_filter_string);
    }
  } else if (option == "--compile-pic") {
    compile_pic_ = true;
  } else if (option.starts_with("--huge-method-max=")) {
    ParseHugeMethodMax(option, Usage);
  } else if (option.starts_with("--large-method-max=")) {
    ParseLargeMethodMax(option, Usage);
  } else if (option.starts_with("--small-method-max=")) {
    ParseSmallMethodMax(option, Usage);
  } else if (option.starts_with("--tiny-method-max=")) {
    ParseTinyMethodMax(option, Usage);
  } else if (option.starts_with("--num-dex-methods=")) {
    ParseNumDexMethods(option, Usage);
  } else if (option.starts_with("--inline-max-code-units=")) {
    ParseInlineMaxCodeUnits(option, Usage);
  } else if (option == "--generate-debug-info" || option == "-g") {
    generate_debug_info_ = true;
  } else if (option == "--no-generate-debug-info") {
    generate_debug_info_ = false;
  } else if (option == "--generate-mini-debug-info") {
    generate_mini_debug_info_ = true;
  } else if (option == "--no-generate-mini-debug-info") {
    generate_mini_debug_info_ = false;
  } else if (option == "--generate-build-id") {
    generate_build_id_ = true;
  } else if (option == "--no-generate-build-id") {
    generate_build_id_ = false;
  } else if (option == "--debuggable") {
    debuggable_ = true;
  } else if (option.starts_with("--top-k-profile-threshold=")) {
    ParseDouble(option.data(), '=', 0.0, 100.0, &top_k_profile_threshold_, Usage);
  } else if (option == "--abort-on-hard-verifier-error") {
    abort_on_hard_verifier_failure_ = true;
  } else if (option.starts_with("--dump-init-failures=")) {
    ParseDumpInitFailures(option, Usage);
  } else if (option.starts_with("--dump-cfg=")) {
    dump_cfg_file_name_ = option.substr(strlen("--dump-cfg=")).data();
  } else if (option == "--dump-cfg-append") {
    dump_cfg_append_ = true;
  } else if (option.starts_with("--register-allocation-strategy=")) {
    ParseRegisterAllocationStrategy(option, Usage);
  } else if (option.starts_with("--verbose-methods=")) {
    // TODO: rather than switch off compiler logging, make all VLOG(compiler) messages
    //       conditional on having verbose methods.
    gLogVerbosity.compiler = false;
    Split(option.substr(strlen("--verbose-methods=")).ToString(), ',', &verbose_methods_);
  } else {
    // Option not recognized.
    return false;
  }
  return true;
}

}  // namespace art