
// Copyright 2023 Victor Smirnov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// This code was adapted from https://github.com/ArthurSonzogni/git-tui/blob/master/src/scroller.cpp
// Copyright 2021 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.

#pragma once

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/event.hpp>



namespace memoria::tests {


class ScrollerBase : public ftxui::ComponentBase {
    int selected_ = 0;
    int size_ = 0;
    ftxui::Box box_;

public:
    ScrollerBase(ftxui::Component child) {
        Add(child);
    }

private:
    ftxui::Element Render() final
    {
        using namespace ftxui;

        auto focused = Focused() ? focus : ftxui::select;
        auto style = Focused() ? inverted : nothing;

        Element background = ComponentBase::Render();
        background->ComputeRequirement();
        size_ = background->requirement().min_y;
        return dbox({
            std::move(background),
            vbox({
                text("") | size(HEIGHT, EQUAL, selected_),
                text("") | style | focused,
            }),
        }) |
        vscroll_indicator | yframe | yflex | reflect(box_);
    }

    bool OnEvent(ftxui::Event event) final
    {
        using namespace ftxui;
        if (event.is_mouse() && box_.Contain(event.mouse().x, event.mouse().y))
            TakeFocus();

        int selected_old = selected_;
        if (event == Event::ArrowUp ||
                (event.is_mouse() && event.mouse().button == Mouse::WheelUp)) {
            selected_--;
        }

        if ((event == Event::ArrowDown ||
             (event.is_mouse() && event.mouse().button == Mouse::WheelDown))) {
            selected_++;
        }

        if (event == Event::PageDown)
            selected_ += box_.y_max - box_.y_min;

        if (event == Event::PageUp)
            selected_ -= box_.y_max - box_.y_min;

        if (event == Event::Home)
            selected_ = 0;

        if (event == Event::End)
            selected_ = size_;

        selected_ = std::max(0, std::min(size_ - 1, selected_));
        return selected_old != selected_;
    }

    bool Focusable() const final { return true; }
};

ftxui::Component Scroller(ftxui::Component child) {
  return Make<ScrollerBase>(std::move(child));
}


}
