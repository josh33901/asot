#pragma once

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using uptr = uintptr_t;

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;
using iptr = intptr_t;

namespace ipc {
	template <typename T>
	T *trailing_allocate(u32 extra) {
		return (T *)(new u8[sizeof(T) + extra]);
	}

	struct Packet {
		u32 from;
		u32 to;
		u32 id;

		// All ipc messages are negative whilst user implemented messages should be positive
		i32 type;
		u32 total_params;
		u32 extra_size;
		// This is trailing allocated
		u8 data[1];
	};

	template <typename T>
	inline auto arg_size(T arg) { return sizeof(T); }

	inline auto arg_size(char *arg) { return strlen(arg) + 1; /*for \0*/ }
	inline auto arg_size(const char *arg) { return strlen(arg) + 1; /*for \0*/ }

	template <typename T>
	inline auto arg_size(T *arg) { return sizeof(T); }

	template <typename T>
	inline auto data_size(T arg) { return arg_size(arg); }

	template <typename T, typename... Args>
	inline auto data_size(T arg, Args... args) { return arg_size(arg) + data_size(args...); }

	template <typename T, typename B>
	inline auto serialize_arg(T arg, B *buffer) {
		auto size = arg_size(arg);
		std::memcpy(buffer, &arg, size);
		return size;
	}

	// This handles char * for us
	template <typename T, typename B>
	inline auto serialize_arg(T *arg, B *buffer) {
		auto size = arg_size(arg);
		std::memcpy(buffer, arg, size);
		return size;
	}

	template <typename T>
	inline void recursive_serialize_arg(Packet *p, u32 starting, T arg) {
		auto starting_pos = &p->data[starting];
		serialize_arg(arg, starting_pos);
	}

	template <typename T, typename... Args>
	inline void recursive_serialize_arg(Packet *p, u32 starting, T arg, Args... args) {
		auto starting_pos = &p->data[starting];
		auto arg_size = serialize_arg(arg, starting_pos);

		recursive_serialize_arg(p, starting + arg_size, args...);
	}

	template <typename... Args>
	inline Packet *create_packet(u32 type, Args... args) {
		auto total_size = data_size(args...);

		auto p = trailing_allocate<Packet>(total_size - 1);
		p->total_params = sizeof...(Args);
		p->extra_size = total_size - 1;
		p->type = type;

		recursive_serialize_arg(p, 0, args...);

		return p;
	}

	template <typename T, typename B>
	inline auto deserialize_arg(T *arg, B *buffer) {
		auto size = arg_size((T *)buffer);
		std::memcpy(arg, buffer, size);
		return size;
	}

	template <typename B>
	inline auto deserialize_arg(char **arg, B *buffer) {
		auto size = arg_size((char *)buffer);
		*arg = new char[size];
		std::memcpy(*arg, buffer, size);
		return size;
	}

	template <typename T>
	inline void recursive_deserialize_arg(Packet *p, u32 starting, T arg) {
		auto starting_pos = &p->data[starting];
		deserialize_arg(arg, starting_pos);
	}

	template <typename T, typename... Args>
	inline void recursive_deserialize_arg(Packet *p, u32 starting, T arg, Args... args) {
		auto starting_pos = &p->data[starting];
		auto arg_size = deserialize_arg(arg, starting_pos);

		recursive_deserialize_arg(p, starting + arg_size, args...);
	}

	template <typename... Args>
	inline void deserialize_packet(Packet *p, Args... args) {
		assert(p->total_params == sizeof...(Args));

		recursive_deserialize_arg(p, 0, args...);
	}

} // namespace ipc
