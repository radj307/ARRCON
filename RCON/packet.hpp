#pragma once
#include "globals.h"

#include <ostream>

namespace packet {
	/// @brief Minimum allowable packet size
	inline constexpr const int PSIZE_MIN{ 10 };
	/// @brief Maximum allowable packet size
	inline constexpr const int PSIZE_MAX{ 10240 };

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
	 * @struct	rc_packet
	 * @brief	Non-serialized RCON Protocol Packet Structure.
	 */
	struct rc_packet {
		int size;
		int id;
		int type;
		std::string body;
		rc_packet() : size{ 0 }, id{ 0 }, type{ 0 }, body{ "" } {}
		rc_packet(const serialized_packet& spacket) : size{ spacket.size }, id{ spacket.id }, type{ spacket.type }, body{ spacket.body } {}
		rc_packet(const int& id, const int& type, const std::string& body) : size{ static_cast<int>(sizeof(int) * 2ull + body.size() + 2ull) }, id{ id }, type{ type }, body{ body } {}
		rc_packet& operator=(const rc_packet& o)
		{
			body.clear();
			size = o.size;
			id = o.id;
			type = o.type;
			body = o.body;
			return *this;
		}

		serialized_packet serialize() const
		{
			serialized_packet s{ 0, 0, 0, {0x00} };
			s.size = size;
			s.id = id;
			s.type = type;
			strncpy_s(s.body, body.c_str(), body.size());
			return s;
		}

		friend std::ostream& operator<<(std::ostream& os, const rc_packet& packet)
		{
			os << g_palette.set(UIElem::PACKET) << packet.body << g_palette.reset();
			if (packet.body.back() != '\n')
				os << '\n'; // print newline if packet doesn't already have one
			os.flush();
			return os;
		}
	};

	/**
	 * @struct	ID_Manager
	 * @brief	Manages command ID codes.
	 */
	static struct {
		static const int ID_MIN{ 1 }, ID_MAX{ INT_MAX - 10 };
	private:
		int _current_id{ ID_MIN };

		constexpr int req()
		{
			if (_current_id >= ID_MAX)
				_current_id = ID_MIN;
			return _current_id;
		}

	public:
		constexpr int get()
		{
			return ++_current_id;
		}

		constexpr int prev() const
		{
			if (_current_id > ID_MIN)
				return _current_id - 1;
			return _current_id;
		}
	} ID_Manager;
}