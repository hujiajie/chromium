// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_GPU_MEDIA_GPU_VIDEO_DECODE_ACCELERATOR_HELPERS_H_
#define CONTENT_COMMON_GPU_MEDIA_GPU_VIDEO_DECODE_ACCELERATOR_HELPERS_H_

#include "base/callback.h"
#include "base/memory/weak_ptr.h"

namespace gfx {
class GLContext;
}

namespace gl {
class GLImage;
}

namespace gpu {
namespace gles2 {
class GLES2Decoder;
}
}

namespace content {

// Helpers/defines for specific VideoDecodeAccelerator implementations in GPU
// process. Which callbacks are required depends on the implementation.
//
// Note that these callbacks may be called more than once, and so must own/share
// ownership of any objects bound to them.
//
// Unless specified otherwise, these callbacks must be executed on the GPU Child
// thread (i.e. the thread which the VDAs are initialized on).

// Return current GLContext.
using GetGLContextCallback = base::Callback<gfx::GLContext*(void)>;

// Make the applicable GL context current. To be called by VDAs before
// executing any GL calls. Return true on success, false otherwise.
using MakeGLContextCurrentCallback = base::Callback<bool(void)>;

// Bind |image| to |client_texture_id| given |texture_target|. If
// |can_bind_to_sampler| is true, then the image may be used as a sampler
// directly, otherwise a copy to a staging buffer is required.
// Return true on success, false otherwise.
using BindGLImageCallback =
    base::Callback<bool(uint32_t client_texture_id,
                        uint32_t texture_target,
                        const scoped_refptr<gl::GLImage>& image,
                        bool can_bind_to_sampler)>;

// Return a WeakPtr to a GLES2Decoder, if one is available.
using GetGLES2DecoderCallback =
    base::Callback<base::WeakPtr<gpu::gles2::GLES2Decoder>(void)>;

}  // namespace content

#endif  // CONTENT_COMMON_GPU_MEDIA_GPU_VIDEO_DECODE_ACCELERATOR_HELPERS_H_
