/**
 * @file	packet.hpp
 * @author	radj307
 * @brief	Contains all of the packet-related objects & methods.
 */
#pragma once
#include <Sequence.hpp>
#include <color-values.h>

#include "globals.h"
#include <ostream>
#include <limits.h>
#include <string.h>


namespace mc_color {
	inline ANSI::Sequence to_sequence(const char& ch)
	{
		using namespace ANSI;
		switch (ch) {
		case '0': // black
			return color::setcolor(color::black);
		case '1': // dark blue
			return color::setcolor(color::dark_blue);
		case '2': // dark green
			return color::setcolor(color::dark_green);
		case '3': // dark aqua
			return color::setcolor(color::dark_cyan);
		case '4': // dark red
			return color::setcolor(color::dark_red);
		case '5': // dark purple
			return color::setcolor(color::dark_purple);
		case '6': // gold
			return color::setcolor(color::gold);
		case '7': // gray
			return color::setcolor(color::gray);
		case '8': // dark gray
			return color::setcolor(color::dark_gray);
		case '9': // blue
			return color::setcolor(color::blue);
		case 'a': // green
			return color::setcolor(color::green);
		case 'b': // aqua
			return color::setcolor(color::cyan);
		case 'c': // red
			return color::setcolor(color::red);
		case 'd': // light purple
			return color::setcolor(color::light_purple);
		case 'e': // yellow
			return color::setcolor(color::yellow);
		case 'f': // white
			return color::setcolor(color::white);
		case 'r': // reset
			return color::reset;
		case 'n': // underline
			return color::underline;
		case 'l': // bold
			return color::bold;
		case 'k': // obfuscated
		case 'm': [[fallthrough]]; // strikethrough
		case 'o': [[fallthrough]]; // italic
		default:
			return{};
		}
	}
}

/**
 * @namespace	packet
 * @brief		Contains the Packet, serialized_packet, and ID_Manager objects.
 */
namespace packet {
	/// @brief Minimum allowable packet size
	inline constexpr const int PSIZE_MIN{ 10 };
	/// @brief Maximum allowable packet size
	inline constexpr const int PSIZE_MAX{ 10240 };
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
		template<class T> requires std::same_as<T, Packet> || std::same_as<T, serialized_packet>
		Packet & operator=(const T & o) noexcept
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
		 * @brief	Retrieve a serialized packet with this packet's data.
		 * @returns	serialized_packet
		 */
		serialized_packet serialize() const
		{
			serialized_packet s{ 0, 0, 0, {0x00} };
			s.size = size;
			s.id = id;
			s.type = type;
			#ifdef OS_WIN
			strncpy_s(s.body, body.c_str(), body.size());
			#else
			strncpy(s.body, body.c_str(), body.size());
			#endif
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

		/**
		 * @brief			Stream insertion operator. Appends a newline to the packet body if one doesn't already exist.
		 * @param os		Output Stream.
		 * @param packet	Packet instance.
		 * @returns			std::ostream&
		 */
		friend std::ostream& operator<<(std::ostream& os, const Packet& packet)
		{
			if (Global.enable_bukkit_color_support) {
				for (auto ch{ packet.body.begin() }; ch != packet.body.end(); ++ch) {
					switch (*ch) {
					case -62: // discard first part of section sign when represented in ASCII
						break;
					case -89: // '§' // second part of ASCII section sign
						if (std::distance(ch, packet.body.end()) > 1ull)
							os << mc_color::to_sequence(*++ch);
						break;
					default:
						os << *ch;
						break;
					}
				}
			}
			else os << Global.palette.set(UIElem::PACKET) << packet.body << Global.palette.reset();
			if (!packet.body.empty() && packet.body.back() != '\n')
				os << '\n'; // print newline if packet doesn't already have one
			return os.flush();
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