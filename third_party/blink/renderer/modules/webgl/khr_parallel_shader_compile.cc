/*
 * Copyright (C) 2018 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/modules/webgl/khr_parallel_shader_compile.h"

#include "gpu/command_buffer/client/gles2_interface.h"

namespace blink {

KHRParallelShaderCompile::KHRParallelShaderCompile(
    WebGLRenderingContextBase* context)
    : WebGLExtension(context) {
  context->ExtensionsUtil()->EnsureExtensionEnabled(
      "GL_KHR_parallel_shader_compile");
  // Use 2 background threads per WebGL context by default.
  context->ContextGL()->MaxShaderCompilerThreadsKHR(2);
}

WebGLExtensionName KHRParallelShaderCompile::GetName() const {
  return kKHRParallelShaderCompileName;
}

KHRParallelShaderCompile* KHRParallelShaderCompile::Create(
    WebGLRenderingContextBase* context) {
  return new KHRParallelShaderCompile(context);
}

void KHRParallelShaderCompile::maxShaderCompilerThreadsKHR(GLuint count) {
  WebGLExtensionScopedContext scoped(this);
  if (scoped.IsLost())
    return;
  // For WebGL contexts, we don't want applications to be able to spin up huge
  // numbers of shader compliation threads. Enforce a maximum of 2 here.
  scoped.Context()->ContextGL()->MaxShaderCompilerThreadsKHR(
      std::max(2u, count));
}

bool KHRParallelShaderCompile::Supported(WebGLRenderingContextBase* context) {
  return context->ExtensionsUtil()->SupportsExtension(
      "GL_KHR_parallel_shader_compile");
}

const char* KHRParallelShaderCompile::ExtensionName() {
  return "KHR_parallel_shader_compile";
}

}  // namespace blink
