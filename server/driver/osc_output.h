/*
 * WiVRn VR streaming
 * Copyright (C) 2022  Guillaume Meunier <guillaume.meunier@centraliens.net>
 * Copyright (C) 2022  Patrick Nicolas <patricknicolas@laposte.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "wivrn_packets.h"
#include <memory>
#include <optional>
#include <string>

namespace wivrn
{
class clock_offset;

class osc_output
{
public:
	osc_output();
	~osc_output();

	// Configure OSC output
	void configure(bool enabled, const std::string & host, int port);

	// Send tracking data in SteamLink OSC format
	void send_tracking(const from_headset::tracking & tracking, const clock_offset & offset);

private:
	struct impl;
	std::unique_ptr<impl> pimpl;
};

} // namespace wivrn

