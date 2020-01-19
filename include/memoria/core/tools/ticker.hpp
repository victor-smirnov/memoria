// Copyright 2016 Victor Smirnov
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


#pragma once

#include <memoria/core/strings/string.hpp>
#include <memoria/core/types.hpp>
#include <memoria/core/tools/time.hpp>

namespace memoria {

class Ticker {
	size_t threshold_value_;
	size_t threshold_;
	size_t ticks_ = 0;
	int64_t start_time_;
	int64_t last_time_;
	int64_t threshold_time_;

public:
	Ticker(size_t threshold): threshold_value_(threshold), threshold_(threshold - 1)
	{
		threshold_time_ = last_time_ = start_time_ = getTimeInMillis();
	}

	bool is_threshold()
	{
		if (threshold_ != ticks_) {
			return false;
		}

		threshold_time_ = getTimeInMillis();

		return true;
	}

	void next()
	{
		last_time_ = threshold_time_;
		threshold_ += threshold_value_;
	}

	int64_t duration() const {
		return threshold_time_ - last_time_;
	}

	size_t ticks() const {return ticks_;}
	size_t size()  const {return threshold_value_;}

	int64_t start_time() const {return start_time_;}

	void tick() {
		ticks_++;
	}
};


}
