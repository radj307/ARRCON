/**
 * @file	packet.hpp
 * @author	radj307
 * @brief	Contains all of the packet-related objects & methods.
 */
#pragma once
#include <ostream>
#include <limits.h>
#include <string.h>

#include <var.hpp>

 /**
  * @namespace	packet
  * @brief		Contains the Packet, serialized_packet, and ID_Manager objects.
  */
namespace net::packet {
	/// @brief Minimum possible RCON-packet size (10 B).
	inline constexpr const int PSIZE_MIN{ 10 };
	/// @brief Maximum allowable packet size (10 kB)
	inline constexpr const int PSIZE_MAX{ 10240 };
	/// @brief Maximum sendable packet size (4 kB)
	inline constexpr const int PSIZE_MAX_SEND{ 4096 };

	/// @brief Minimum allowable packet ID number.
	inline constexpr const int PID_MIN{ 1 };
	/// @brief Maximum allowable packet ID number.
	inline constexpr const int PID_MAX{ INT_MAX / 2 };

	/**
	 * @struct	Type
	 * @brief	Represents an RCON packet's type.
	 */
	struct Type {
	private:
		const int _type;
		constexpr Type(const int& type) : _type{ type } {}
	public:
		constexpr bool operator==(const Type& o) const { return _type == o._type; }
		constexpr bool operator==(const int& type_index) const { return _type == type_index; }
		constexpr bool operator!=(auto&& o) const { return !operator==(std::forward<decltype(o)>(o)); }
		constexpr operator const int() const { return _type; }
		static const Type
			/// @brief RCON Authorization Request
			SERVERDATA_AUTH,
			/// @brief RCON Authorization Response
			SERVERDATA_AUTH_RESPONSE,
			/// @brief RCON Command Request
			SERVERDATA_EXECCOMMAND,
			/// @brief RCON Command Response
			SERVERDATA_RESPONSE_VALUE;
	};
	// RCON Authorization Request
	inline constexpr const Type Type::SERVERDATA_AUTH{ 3 };
	// RCON Authorization Response
	inline constexpr const Type Type::SERVERDATA_AUTH_RESPONSE{ 2 };
	// RCON Command Request
	inline constexpr const Type Type::SERVERDATA_EXECCOMMAND{ 2 };
	// RCON Command Response
	inline constexpr const Type Type::SERVERDATA_RESPONSE_VALUE{ 0 };

	/**
	 * @struct	serialize
	 * @brief	Used to serialize & unserialize packet data when sending/receiving data over the socket.
	 */
	typedef struct {
		int size;
		int id;
		int type;
		char body[PSIZE_MAX];
	} serialized_packet;

	/**
	 * @struct	Packet
	 * @brief	Non-serialized RCON Protocol Packet Structure.
	 */
	struct Packet {
		int size;			///< @brief Packet Size
		int id;				///< @brief Packet ID
		int type;			///< @brief Packet Type
		std::string body;	///< @brief Packet Body

		/**
		 * @brief	Zeroed/Null Constructor.
		 */
		Packet() : size{ NULL }, id{ NULL }, type{ NULL } {}
		/**
		 * @brief			Unserialize Constructor.
		 * @param spacket	A serialized_packet object to copy values from.
		 */
		Packet(const serialized_packet& spacket) : size{ spacket.size }, id{ spacket.id }, type{ spacket.type }, body{ spacket.body } {}
		/**
		 * @brief		Constructor.
		 * @param id	Packet ID number. (Must be greater than 1 to be valid)
		 * @param type	Packet Type. Accepts the integral value or a Type object.
		 * @param body	Packet Body / Message String
		 */
		Packet(const int& id, const int& type, const std::string& body) : size{ static_cast<int>(sizeof(int) * 2ull + body.size() + 2ull) }, id{ id }, type{ type }, body{ body } {}

		/**
		 * @brief		Packet assignment operator.
		 * @tparam T	Input Type.
		 * @param o		Another Packet or serialized_packet instance.
		 * @returns		Packet&
		 */
		template<var::any_same<Packet, serialized_packet> T>
		Packet& operator=(const T& o) noexcept
		{
			body.clear();
			size = o.size;
			id = o.id;
			type = o.type;
			body = o.body;
			return *this;
		}

		/**
		 * @brief	Check if this Packet has valid member values.
		 * @returns	bool
		 */
		bool isValid() const
		{
			return (size > PSIZE_MIN && size < PSIZE_MAX) && (id >= PID_MIN && id <= PID_MAX) && (type == 0 || type == 2 || type == 3);
		}

		/**
		 * @brief	Check if this packet's body is a valid length
		 * @returns	bool
		 */
		bool isValidLength() const
		{
			return size >= 0 && size < PSIZE_MAX_SEND;
		}

		/**
		 * @brief	Retrieve a serialized packet with this packet's data.
		 * @returns	serialized_packet
		 */
		serialized_packet serialize() const
		{
			serialized_packet s{};
			s.size = size;
			s.id = id;
			s.type = type;
			strncpy(s.body, body.c_str(), body.size()); //< don't use strncpy_s on windows or it sets the remaining packet bytes to -2 (0xFE) instead of 0
			return s;
		}

		/**
		 * @brief	Zeroes all parameters, effectively resetting the packet data.
		 */
		void zero()
		{
			size = 0;
			id = 0;
			type = 0;
			body.clear();
		}
	};

	/**
	 * @struct	ID_Manager
	 * @brief	Manages packet ID codes, which are used to match responses to the request that provoked it.
	 */
	static struct {
	private:
		/// @brief Tracks the last used packet ID number.
		int _current_id{ PID_MIN };

	public:
		/**
		 * @brief	Retrieve a unique packet ID number.
		 *\n		Numbers are unique so far as they can be guaranteed to not match
		 *\n		any packets still being processed at the same time when the number
		 *\n		of responses per request is smaller than (INT_MAX / 2).
		 * @returns	int
		 */
		constexpr int get()
		{
			if (_current_id + 1 < PID_MAX) // if id is in range, increment it and return
				return ++_current_id;
			return _current_id = PID_MIN; // else, loop id back to the minimum bound of the valid ID range
		}
	} ID_Manager;
}