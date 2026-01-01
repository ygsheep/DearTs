/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#pragma once

/**
 * Wrapper header to fix namespace issues with Live2D Cubism Core
 * 
 * This header provides proper namespace declarations for the Core types
 * without modifying the original Live2D Framework source code.
 */

// First include the original header
#include "../../../third_party/Live2D/Core/include/Live2DCubismCore.h"

// Then create the namespace with the types
namespace Live2D { namespace Cubism { namespace Core {
    // Import the C types into the Core namespace
    typedef ::csmLogFunction csmLogFunction;
    typedef ::csmParameterType csmParameterType;
    typedef ::csmVector2 csmVector2;
    typedef ::csmVector4 csmVector4;
    typedef ::csmModel csmModel;
    typedef ::csmMoc csmMoc;
    typedef ::csmMocVersion csmMocVersion;
}}}