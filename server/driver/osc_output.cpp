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

#include "osc_output.h"
#include "clock_offset.h"
#include "wivrn_packets.h"
#include "configuration.h"
#include <arpa/inet.h>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <util/u_logging.h>
#include <vector>

namespace wivrn
{

// Minimal OSC message builder
class osc_message
{
	std::vector<uint8_t> data;
	std::string type_tag = ",";

	void pad_string(const std::string & str)
	{
		data.insert(data.end(), str.begin(), str.end());
		data.push_back(0);
		// Pad to 4-byte boundary
		while (data.size() % 4 != 0)
			data.push_back(0);
	}

public:
	osc_message(const std::string & address)
	{
		pad_string(address);
	}

	void add_float(float value)
	{
		type_tag += 'f';
		uint32_t bits;
		std::memcpy(&bits, &value, sizeof(float));
		bits = htonl(bits);
		data.insert(data.end(), reinterpret_cast<uint8_t *>(&bits), reinterpret_cast<uint8_t *>(&bits) + sizeof(uint32_t));
	}

	void add_int32(int32_t value)
	{
		type_tag += 'i';
		uint32_t bits = htonl(static_cast<uint32_t>(value));
		data.insert(data.end(), reinterpret_cast<uint8_t *>(&bits), reinterpret_cast<uint8_t *>(&bits) + sizeof(uint32_t));
	}

	std::vector<uint8_t> finish()
	{
		// Add type tag string
		pad_string(type_tag);
		return data;
	}
};

struct osc_output::impl
{
	bool enabled = false;
	std::string host = "127.0.0.1";
	int port = 9000;
	int socket_fd = -1;
	sockaddr_in addr{};

	impl()
	{
		socket_fd = ::socket(AF_INET, SOCK_DGRAM, 0);
		if (socket_fd < 0)
		{
			U_LOG_E("Failed to create OSC socket: %s", strerror(errno));
		}
	}

	~impl()
	{
		if (socket_fd >= 0)
			::close(socket_fd);
	}

	void configure(bool enable, const std::string & new_host, int new_port)
	{
		enabled = enable;
		host = new_host;
		port = new_port;

		if (socket_fd < 0)
			return;

		std::memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);

		if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) <= 0)
		{
			U_LOG_E("Invalid OSC host address: %s", host.c_str());
			enabled = false;
			return;
		}

		if (enabled)
		{
			U_LOG_I("OSC output enabled: %s:%d", host.c_str(), port);
		}
	}

	void send(const std::vector<uint8_t> & message)
	{
		if (!enabled || socket_fd < 0)
			return;

		ssize_t sent = sendto(socket_fd, message.data(), message.size(), 0, reinterpret_cast<sockaddr *>(&addr), sizeof(addr));
		if (sent < 0)
		{
			U_LOG_W("Failed to send OSC message: %s", strerror(errno));
		}
	}

	std::string device_name(device_id device)
	{
		// Map device_id to SteamLink-compatible tracker names
		switch (device)
		{
			case device_id::HEAD:
				return "head";
			case device_id::LEFT_GRIP:
			case device_id::LEFT_AIM:
			case device_id::LEFT_PALM:
				return "left_controller";
			case device_id::RIGHT_GRIP:
			case device_id::RIGHT_AIM:
			case device_id::RIGHT_PALM:
				return "right_controller";
			case device_id::EYE_GAZE:
				return "eye_gaze";
			default:
				// For unknown devices, use the enum name
				return "device_" + std::to_string(static_cast<int>(device));
		}
	}
};

osc_output::osc_output() : pimpl(std::make_unique<impl>())
{
}

osc_output::~osc_output() = default;

void osc_output::configure(bool enabled, const std::string & host, int port)
{
	pimpl->configure(enabled, host, port);
}

void osc_output::send_tracking(const from_headset::tracking & tracking, const clock_offset & offset)
{
	if (!pimpl->enabled)
		return;

	// Send tracking data for each device pose
	for (const auto & pose : tracking.device_poses)
	{
		std::string device_name = pimpl->device_name(pose.device);
		std::string base_path = "/tracking/trackers/" + device_name;

		// Send position if valid
		if (pose.flags & from_headset::tracking::flags::position_valid)
		{
			osc_message msg(base_path + "/position");
			msg.add_float(pose.pose.position.x);
			msg.add_float(pose.pose.position.y);
			msg.add_float(pose.pose.position.z);
			pimpl->send(msg.finish());
		}

		// Send rotation (quaternion) if valid
		if (pose.flags & from_headset::tracking::flags::orientation_valid)
		{
			osc_message msg(base_path + "/rotation");
			msg.add_float(pose.pose.orientation.x);
			msg.add_float(pose.pose.orientation.y);
			msg.add_float(pose.pose.orientation.z);
			msg.add_float(pose.pose.orientation.w);
			pimpl->send(msg.finish());
		}

		// Send linear velocity if valid
		if (pose.flags & from_headset::tracking::flags::linear_velocity_valid)
		{
			osc_message msg(base_path + "/velocity");
			msg.add_float(pose.linear_velocity.x);
			msg.add_float(pose.linear_velocity.y);
			msg.add_float(pose.linear_velocity.z);
			pimpl->send(msg.finish());
		}

		// Send angular velocity if valid
		if (pose.flags & from_headset::tracking::flags::angular_velocity_valid)
		{
			osc_message msg(base_path + "/angularvelocity");
			msg.add_float(pose.angular_velocity.x);
			msg.add_float(pose.angular_velocity.y);
			msg.add_float(pose.angular_velocity.z);
			pimpl->send(msg.finish());
		}
	}
}

} // namespace wivrn

