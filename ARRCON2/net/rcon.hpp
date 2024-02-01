#pragma once
#include "../logging.hpp"

// Boost::asio
#include <boost/asio.hpp>

// STL
#include <cstdint>	//< for sized integer types
#include <vector>	//< for std::vector
#include <string>	//< for std::string
#include <iostream>	//< for std::clog

namespace net {
	using boost::asio::io_context;
	using boost::asio::ip::tcp;

	/**
	 * @brief				Resolves a target endpoint from the specified host and port.
	 * @param io_context  -	The io_context to use.
	 * @param host		  -	The target hostname.
	 * @param port		  -	The target port number.
	 * @returns				The resolved target when successful; otherwise, std::nullopt.
	 */
	tcp::resolver::results_type resolve_targets(io_context& io_context, std::string_view host, std::string_view port)
	{
		return tcp::resolver(io_context).resolve(host, port);
	}

	namespace rcon {
		enum class PacketType : int32_t {
			SERVERDATA_AUTH = 3,
			SERVERDATA_AUTH_RESPONSE = 2,
			SERVERDATA_EXECCOMMAND = 2,
			SERVERDATA_RESPONSE_VALUE = 0,
		};

		inline constexpr const int32_t PACKETID_MIN{ 1 };
		inline constexpr const int32_t PACKETID_MAX{ std::numeric_limits<int32_t>::max() };

		struct packet_header {
			int32_t size{ 0 };
			int32_t id{ 0 };
			int32_t type{ 0 };
		};

		inline constexpr int32_t get_packet_size(size_t const bodySize)
		{
			// 4 packet size bytes aren't included vvvvvvv
			return (sizeof(packet_header) - sizeof(int32_t)) + bodySize + 2;
		}

		/// @brief	Minimum possible size of an RCON packet.
		inline constexpr const int32_t PACKETSZ_MIN{ sizeof(packet_header) + 2 };
		/// @brief	Maximum number of bytes that can be sent in a single packet, before being split between multiple packets.
		inline constexpr const int32_t PACKETSZ_MAX_SEND{ 4096 };

		inline std::string bytes_to_string(std::vector<uint8_t> const& bytes)
		{
			std::string s{ bytes.size(), 0, std::allocator<char>() };

			std::memcpy(const_cast<char*>(s.c_str()), bytes.data(), bytes.size());

			return s;
		}

		/// @brief	Source RCON client object.
		class RconClient {
			using buffer = std::vector<uint8_t>;

			io_context ioContext;
			tcp::socket socket;
			int32_t currentPacketid{ PACKETID_MIN };

			/**
			 * @brief	Gets the next pseudo-unique packet ID.
			 * @returns	A pseudo-unique packet ID number.
			 */
			int32_t get_next_packet_id()
			{
				if (currentPacketid == PACKETID_MAX)
					currentPacketid = PACKETID_MIN;
				return currentPacketid++;
			}

			/**
			 * @brief			Creates a packet buffer from the specified header and body.
			 * @param header  -	The pre-constructed packet header to use.
			 * @param body	  -	The body string to use.
			 * @returns			A buffer containing the packet's raw bytes.
			 */
			buffer build_packet(packet_header const& header, std::string const& body)
			{
				// create a buffer with 2 extra bytes for the packet terminator bytes
				buffer buf(sizeof(packet_header) + body.size() + 2, 0, std::allocator<uint8_t>());

				// copy the buffer header into the buffer
				std::memcpy(&buf[0], &header, sizeof(packet_header));

				// copy the buffer body into the buffer
				std::memcpy(&buf[0] + sizeof(packet_header), body.c_str(), body.size());

				return buf;
			}

			buffer build_terminator_packet(int32_t const id)
			{
				return build_packet(packet_header{ get_packet_size(0), id, (int32_t)PacketType::SERVERDATA_RESPONSE_VALUE }, "");
			}

			/**
			 * @brief	Receives a single packet.
			 * @returns	A pair containing the packet header and the packet body.
			 */
			std::pair<packet_header, buffer> recv()
			{
				// read the packet header
				packet_header header{};
				boost::asio::mutable_buffer buf(&header, sizeof(packet_header));
				boost::asio::read(socket, buf); //< TODO: validate received byte count

				// read the packet body
				const auto avail{ socket.available() };
				const auto bodySize{ header.size - (sizeof(packet_header) - sizeof(int32_t)) };
				buffer body_buffer{ bodySize, 0, std::allocator<uint8_t>() };
				boost::asio::read(socket, boost::asio::buffer(body_buffer)); //< TODO: validate received byte count

				// remove the null terminators from the body buffer
				body_buffer.erase(std::remove(body_buffer.begin(), body_buffer.end(), '\0'), body_buffer.end());

				return std::make_pair(header, body_buffer);
			}

			void connect(std::string_view host, std::string_view port)
			{
				// resolve the target endpoint
				tcp::resolver::results_type targets;
				try {
					targets = resolve_targets(ioContext, host, port);

					if (const auto targetCount{ targets.size() }; targetCount > 0) {
						// write a success log message
						std::clog << MessageHeader(LogLevel::Debug) << "Successfully resolved " << targetCount << " endpoint" << (targetCount != 1 ? "s" : "") << " for target \"" << host << ':' << port << '\"' << std::endl;
					}
					else {
						// target resolution failed; log the error & throw
						std::clog << MessageHeader(LogLevel::Error) << "Failed to resolve target \"" << host << ':' << port << '\"' << std::endl;

						throw make_exception("Failed to resolve target \"", host, ':', port, "\"!");
					}
				} catch (std::exception const& ex) {
					// target resolution threw an exception; log the error & throw
					std::clog << MessageHeader(LogLevel::Error) << "Failed to resolve target \"" << host << ':' << port << "\" due to exception: \"" << ex.what() << "\"" << std::endl;

					throw make_exception("Failed to resolve target \"", host, ':', port, "\" due to exception: \"", ex.what(), '\"');
				}

				// connect the socket to the endpoint
				try {
					const auto endpoint{ boost::asio::connect(socket, targets) }; //< connect() throws on error

					std::clog << MessageHeader(LogLevel::Debug) << "Successfully connected to endpoint \"" << endpoint << '\"' << std::endl;;
				} catch (std::exception const& ex) {
					std::clog << MessageHeader(LogLevel::Error) << "Failed to connect to target \"" << host << ':' << port << "\" due to exception: \"" << ex.what() << '\"' << std::endl;

					throw make_exception("Failed to connect to target \"", host, ':', port, "\". Original exception message: ", ex.what());
				}
			}

		public:
			RconClient(std::string_view host, std::string_view port) noexcept(false) : socket{ ioContext }
			{
				connect(host, port);
			}
			~RconClient()
			{
				ioContext.run(); //< wait for async operations to finish
				socket.close(); //< close the socket
			}

			/**
			 * @brief				Sends a command to the RCON server and returns the response.
			 * @param command	  -	The command to send to the RCON server.
			 * @returns				The response from the RCON server when successful.
			 */
			std::string command(std::string const& command) noexcept(false)
			{
				const auto packetId{ get_next_packet_id() };
				const buffer packet{ build_packet(packet_header{ get_packet_size(command.size()), packetId, (int32_t)PacketType::SERVERDATA_EXECCOMMAND }, command) };

				if (boost::asio::write(socket, boost::asio::buffer(packet)) != packet.size()) {
					std::clog << MessageHeader(LogLevel::Error) << "Failed to send command packet \"" << command << "\" with id " << packetId << " & size " << packet.size() << "" << std::endl;
					throw make_exception("Failed to send packet for command \"", command, "\"!");
				}

				std::pair<packet_header, buffer> response{ recv() };

				// build & send a terminator packet
				const auto termPacketId{ get_next_packet_id() };
				const buffer termPacket{ build_terminator_packet(termPacketId) };

				if (boost::asio::write(socket, boost::asio::buffer(termPacket)) != termPacket.size()) {
					std::clog << MessageHeader(LogLevel::Error) << "Failed to send terminator packet for command \"" << command << "\" with id " << termPacketId << " & size " << termPacket.size() << std::endl;
					throw make_exception("Failed to send terminator packet for command \"", command, "\"!");
				}

				// receive & concatenate packets until the terminator packet is reached
				std::stringstream ss{ bytes_to_string(response.second) };

				for (; response.first.id == packetId; response = recv()) {
					ss << bytes_to_string(response.second);
				}

				if (response.first.id != termPacketId) {
					std::clog << MessageHeader(LogLevel::Warning) << "Did not receive the terminator packet (" << termPacketId << ") for command \"" << command << "\"! Flushing the buffer." << std::endl;
					flush();
				}

				return ss.str();
			}

			bool authenticate(std::string_view password)
			{
				const buffer p{ build_packet(packet_header{ get_packet_size(password.size()), 1, (int32_t)PacketType::SERVERDATA_AUTH }, password.data()) };

				if (boost::asio::write(socket, boost::asio::buffer(p)) != p.size()) {
					std::clog << MessageHeader(LogLevel::Error) << "Failed to send authentication packet!" << std::endl;

					return false;
				}

				// receive response
				const auto& [auth_response_header, _] { recv() };

				if (auth_response_header.id == -1) {
					throw make_exception("Authentication refused by server! (Incorrect password)");
				}
				else return true;
			}

			void flush()
			{
				const auto bytes{ socket.available() };
				if (bytes == 0) return;

				buffer p{ bytes, 0, std::allocator<uint8_t>() };
				boost::asio::read(socket, boost::asio::buffer(p));
			}
		};
	}
}
