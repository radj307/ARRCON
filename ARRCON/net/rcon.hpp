#pragma once
#include "../logging.hpp"
#include "../exceptions.hpp"

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
			int32_t size{};
			int32_t id{};
			int32_t type{};
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

		/**
		 * @brief			Converts the specified vector of bytes to a string by direct copying.
		 * @param bytes	  -	A vector of bytes to convert to a readable string.
		 * @returns			The string representation of the specified bytes.
		 */
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
			/// @brief	Builds a special blank terminator packet with the specified id.
			buffer build_terminator_packet(int32_t const id)
			{
				return build_packet(packet_header{ get_packet_size(0), id, (int32_t)PacketType::SERVERDATA_RESPONSE_VALUE }, "");
			}

			/**
			 * @brief		Sends a blank message terminator packet to the server.
			 * @param ec  -	error_code reference to use for socket errors
			 * @returns		The ID of the terminator packet.
			 */
			int32_t send_terminator_packet(boost::system::error_code& ec)
			{
				const int32_t termPacketId{ get_next_packet_id() };
				const buffer termPacket{ build_terminator_packet(termPacketId) };

				// send the terminator packet to the server
				if (boost::asio::write(socket, boost::asio::buffer(termPacket), ec) != termPacket.size())
					return -1;

				return termPacketId;
			}

			/**
			 * @brief	Receives a single packet.
			 * @returns	A pair containing the packet header and the packet body.
			 */
			std::pair<packet_header, buffer> recv() noexcept(false)
			{
				// error code
				boost::system::error_code ec{};

				// read the packet header
				packet_header header{};
				boost::asio::mutable_buffer buf(&header, sizeof(packet_header));
				boost::asio::read(socket, buf, ec); //< TODO: validate received byte count

				// check for errors
				if (ec) {
					const auto error_message{ str::stringify("Failed to read packet header due to error: \"", ec.what(), "\"! Flushing the buffer.") };
					std::clog << MessageHeader(LogLevel::Error) << error_message << std::endl;

					flush();

					throw_with_stacktrace(make_exception(error_message));
				}

				// read the packet body
				const auto avail{ socket.available() };
				const auto bodySize{ header.size - (sizeof(packet_header) - sizeof(int32_t)) };
				buffer body_buffer{ bodySize, 0, std::allocator<uint8_t>() };
				boost::asio::read(socket, boost::asio::buffer(body_buffer), ec); //< TODO: validate received byte count

				// check for errors
				if (ec) {
					const auto error_message{ str::stringify("Failed to read packet body due to error: \"", ec.what(), "\"! Flushing the buffer.") };
					std::clog << MessageHeader(LogLevel::Error) << error_message << std::endl;

					flush();

					throw_with_stacktrace(make_exception(error_message));
				}

				// remove the null terminators from the body buffer
				body_buffer.erase(std::remove(body_buffer.begin(), body_buffer.end(), '\0'), body_buffer.end());

				return std::make_pair(header, body_buffer);
			}

			/// @brief	Connects the RCON client to the specified endpoint.
			void connect(std::string_view host, std::string_view port) noexcept(false)
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

						throw_with_stacktrace(make_exception("Failed to resolve target \"", host, ':', port, "\"!"));
					}
				} catch (std::exception const& ex) {
					// target resolution threw an exception; log the error & throw
					std::clog << MessageHeader(LogLevel::Error) << "Failed to resolve target \"" << host << ':' << port << "\" due to exception: \"" << ex.what() << "\"" << std::endl;

					throw_with_stacktrace(make_exception("Failed to resolve target \"", host, ':', port, "\" due to exception: \"", ex.what(), '\"'));
				}

				// connect the socket to the endpoint
				try {
					const auto endpoint{ boost::asio::connect(socket, targets) }; //< connect() throws on error

					std::clog << MessageHeader(LogLevel::Debug) << "Successfully connected to endpoint \"" << endpoint << '\"' << std::endl;;
				} catch (std::exception const& ex) {
					std::clog << MessageHeader(LogLevel::Error) << "Failed to connect to target \"" << host << ':' << port << "\" due to exception: \"" << ex.what() << '\"' << std::endl;

					throw_with_stacktrace(make_exception("Failed to connect to target \"", host, ':', port, "\". Original exception message: ", ex.what()));
				}
			}

		public:
			/**
			 * @brief			Creates a new RconClient instance and connects it to the specified endpoint.
			 * @param host    -	The hostname of the target endpoint.
			 * @param port	  -	The port of the target endpoint.
			 */
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
				boost::system::error_code ec{};

				// build the command packet
				const auto packetId{ get_next_packet_id() };
				const buffer packet{ build_packet(packet_header{ get_packet_size(command.size()), packetId, (int32_t)PacketType::SERVERDATA_EXECCOMMAND }, command) };

				// send the command packet to the server
				if (const auto sent_bytes{ boost::asio::write(socket, boost::asio::buffer(packet), ec) };
					sent_bytes != packet.size() || ec) {
					// an error occurred:
					const auto error_message{
						sent_bytes == packet.size()
						? str::stringify("Sent ", sent_bytes, '/', packet.size(), " bytes of packet #", packetId, " with command \"", command, "\", but an error occurred: ", ec.what())
						: str::stringify("Sent ", sent_bytes, '/', packet.size(), " bytes of packet #", packetId, " with command \"", command, "\" due to error: ", ec.what())
					};

					std::clog << MessageHeader(LogLevel::Error) << error_message << std::endl;
					throw_with_stacktrace(make_exception(error_message));
				}

				std::clog << MessageHeader(LogLevel::Debug) << "Sent packet #" << packetId << " with command \"" << command << '\"' << std::endl;

				// send the message terminator packet
				const int32_t termPacketId{ send_terminator_packet(ec) };

				std::stringstream responseBody;
				int32_t receivedPackets{ 0 };
				std::pair<packet_header, buffer> response;

				// receive the response
				for (response = recv(), receivedPackets = 1;
					 response.first.id == packetId;
					 response = recv(), ++receivedPackets) {
					responseBody << bytes_to_string(response.second);
				}

				std::clog                   // subtract 1 because of terminator packet  vvv
					<< MessageHeader(LogLevel::Debug) << "Received " << receivedPackets - 1 << " response packet" << (receivedPackets == 1 ? "" : "s") << '.' << std::endl;

				return responseBody.str();
			}

			/**
			 * @brief				Authenticates with the connected RCON server by sending the specified password.
			 * @param password	  -	The password to send to the server.
			 * @returns				True when successful; otherwise, false.
			 */
			bool authenticate(std::string_view password)
			{
				boost::system::error_code ec{};

				const buffer p{ build_packet(packet_header{ get_packet_size(password.size()), 1, (int32_t)PacketType::SERVERDATA_AUTH }, password.data()) };

				if (boost::asio::write(socket, boost::asio::buffer(p), ec) != p.size()) {
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

			/// @brief	Empties the buffer and returns its contents.
			buffer flush()
			{
				const auto bytes{ socket.available() };
				if (bytes == 0) return {};

				buffer p{ bytes, 0, std::allocator<uint8_t>() };
				boost::asio::read(socket, boost::asio::buffer(p));
				return p;
			}

			/**
			 * @brief				Sets the socket timeout duration in milliseconds.
			 * @param timeout_ms  -	Number of milliseconds to wait for a response before timing out.
			 */
			void set_timeout(int timeout_ms)
			{
				socket.set_option(boost::asio::detail::socket_option::integer<SOL_SOCKET, SO_RCVTIMEO>{ timeout_ms });
			}

			/**
			 * @brief		Gets the current size of the socket's data buffer.
			 * @returns		The number of bytes that haven't been read from the buffer yet.
			 */
			size_t buffer_size()
			{
				return socket.available();
			}
		};
	}
}
