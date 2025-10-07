#include "layout_base.h"
#include "../window_base.h"

namespace DearTs {
namespace Core {
namespace Window {

/**
 * LayoutBase构造函数
 */
LayoutBase::LayoutBase(const std::string& name)
    : name_(name)
    , parentWindow_(nullptr)
    , visible_(true)
    , x_(0.0f)
    , y_(0.0f)
    , width_(0.0f)
    , height_(0.0f) {
}

} // namespace Window
} // namespace Core
} // namespace DearTs