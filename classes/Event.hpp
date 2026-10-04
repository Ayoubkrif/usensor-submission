#pragma once

# include "types.hpp"
# include <ostream>

enum class EventType : u8
{
	Camera = 1,
	Imu = 2,
	Gps = 3,
	Temp = 4,
	Button = 5,
};

inline const char	*toString(EventType type)
{
	switch (type) {
		case EventType::Camera:
			return "camera";
		case EventType::Imu:
			return "imu";
		case EventType::Gps:
			return "gps";
		case EventType::Temp:
			return "temp";
		case EventType::Button:
			return "button";
	}
	return "unknown";
}

// Base virtual class for events
class Event
{
	public:
		virtual ~Event() = 0;

		u64			timestamp() const { return _timestamp; }
		EventType	type() const { return _type; }
		// comp overload
		bool	operator<(const Event &other) const { return _timestamp < other._timestamp; }
	protected:
		Event(EventType type, u64 timestamp) : _timestamp(timestamp), _type(type) {}
	private:
		u64			_timestamp;
		EventType	_type;
};

inline Event::~Event() {}

inline std::ostream	&operator<<(std::ostream &os, const Event &event) {
	return os << "{\"type\":\"" << toString(event.type())
		<< "\",\"timestamp_ns\":" << event.timestamp() << '}';
}

class CameraEvent final : public Event {
	public:
		explicit CameraEvent(u64 timestamp) : Event(EventType::Camera, timestamp) {}
};

class ImuEvent final : public Event {
	public:
		explicit ImuEvent(u64 timestamp) : Event(EventType::Imu, timestamp) {}
};

class GpsEvent final : public Event {
	public:
		explicit GpsEvent(u64 timestamp) : Event(EventType::Gps, timestamp) {}
};

class TempEvent final : public Event {
	public:
		explicit TempEvent(u64 timestamp) : Event(EventType::Temp, timestamp) {}
};

class ButtonEvent final : public Event {
	public:
		explicit ButtonEvent(u64 timestamp) : Event(EventType::Button, timestamp) {}
};
