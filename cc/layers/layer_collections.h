// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_LAYERS_LAYER_COLLECTIONS_H_
#define CC_LAYERS_LAYER_COLLECTIONS_H_

#include <unordered_map>
#include <vector>

#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "cc/base/cc_export.h"

namespace cc {
class Layer;
class LayerImpl;

using LayerList = std::vector<scoped_refptr<Layer>>;
using OwnedLayerImplList = std::vector<scoped_ptr<LayerImpl>>;
using LayerImplList = std::vector<LayerImpl*>;
using OwnedLayerImplMap = std::unordered_map<int, scoped_ptr<LayerImpl>>;
using LayerImplMap = std::unordered_map<int, LayerImpl*>;

}  // namespace cc

#endif  // CC_LAYERS_LAYER_COLLECTIONS_H_
