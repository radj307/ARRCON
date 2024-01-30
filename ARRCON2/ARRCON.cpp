#include "version.h"
#include "copyright.h"

//#include "net/RconClient.hpp"

// 307lib
#include <opt3.hpp>
#include <color-sync.hpp>	//< for color::sync

// STL
#include <filesystem>	//< for std::filesystem
#include <iostream>		//< for standard io streams

struct print_help {
	std::string exeName;

	print_help(const std::string& exeName) : exeName{ exeName } {}

	friend std::ostream& operator<<(std::ostream& os, const print_help& h)
	{
		return os << h.exeName << " v" << ARRCON_VERSION_EXTENDED << " (" << ARRCON_COPYRIGHT << ")\n"
			<< "  A robust & powerful commandline Remote-CONsole (RCON) client designed for use with the Source RCON Protocol.\n"
			<< "  It is also compatible with similar protocols such as the one used by Minecraft.\n"
			<< '\n'
			<< "  Report compatibility issues here: https://github.com/radj307/ARRCON/issues/new?template=support-request.md\n"
			<< '\n'
			<< "USAGE:" << '\n'
			<< "  " << h.exeName << " [OPTIONS] [COMMANDS]\n"
			<< '\n'
			<< "  Some arguments take additional inputs, labeled with <angle brackets>." << '\n'
			<< "  Arguments that contain spaces must be enclosed with single (\') or double(\") quotation marks." << '\n'
			<< '\n'
			<< "TARGET SPECIFIER OPTIONS:\n"
			<< "  -H, --host  <Host>          RCON Server IP/Hostname.  (Default: \"" /*<< Global.DEFAULT_TARGET.hostname*/ << "\")" << '\n'
			<< "  -P, --port  <Port>          RCON Server Port.         (Default: \"" /*<< Global.DEFAULT_TARGET.port*/ << "\")" << '\n'
			<< "  -p, --pass  <Pass>          RCON Server Password." << '\n'
			<< "  -S, --saved <Host>          Use a saved host's connection information, if it isn't overridden by arguments." << '\n'
			<< "      --save-host <H>         Create a new saved host named \"<H>\" using the current [Host/Port/Pass] value(s)." << '\n'
			<< "      --remove-host <H>       Remove an existing saved host named \"<H>\" from the list, then exit." << '\n'
			<< "  -l, --list-hosts            Show a list of all saved hosts, then exit." << '\n'
			<< '\n'
			<< "OPTIONS:\n"
			<< "  -h, --help                  Show the help display, then exit." << '\n'
			<< "  -v, --version               Print the current version number, then exit." << '\n'
			<< "  -q, --quiet                 Silent/Quiet mode; prevents or minimizes console output." << '\n'
			<< "  -i, --interactive           Starts an interactive command shell after sending any scripted commands." << '\n'
			<< "  -w, --wait <ms>             Wait for \"<ms>\" milliseconds between sending each command in mode [2]." << '\n'
			<< "  -n, --no-color              Disable colorized console output." << '\n'
			<< "  -Q, --no-prompt             Disables the prompt in interactive mode, and command echo in commandline mode." << '\n'
			<< "      --print-env             Prints all recognized environment variables, their values, and descriptions." << '\n'
			<< "      --write-ini             (Over)write the INI file with the default configuration values & exit." << '\n'
			<< "      --update-ini            Writes the current configuration values to the INI file, and adds missing keys." << '\n'
			<< "  -f, --file <file>           Load the specified file and run each line as a command." << '\n'
			;
	}
};

// terminal color synchronizer
color::sync csync{};

#include <boost/asio.hpp>

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

		class RconClient {
			using buffer = std::vector<uint8_t>;

			io_context ioContext;
			tcp::socket socket;
			int32_t current_packetid{ PACKETID_MIN };

			/**
			 * @brief	Gets the next pseudo-unique packet ID.
			 * @returns	A pseudo-unique packet ID number.
			 */
			int32_t get_next_packet_id()
			{
				if (current_packetid == PACKETID_MAX)
					current_packetid = PACKETID_MIN;
				return current_packetid++;
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
				const auto bodySize{ header.size - (sizeof(packet_header) - sizeof(int32_t)) };
				buffer body_buffer{ bodySize, 0, std::allocator<uint8_t>() };
				boost::asio::read(socket, boost::asio::buffer(body_buffer)); //< TODO: validate received byte count

				// remove the null terminators from the body buffer
				body_buffer.erase(std::remove(body_buffer.begin(), body_buffer.end(), '\0'), body_buffer.end());

				return std::make_pair(header, body_buffer);
			}

		public:
			RconClient(std::string_view host, std::string_view port, std::string_view password) noexcept(false) : socket{ ioContext }
			{
				// resolve the target endpoint
				const auto targets{ resolve_targets(ioContext, host, port) };

				// connect the socket
				try {
					const auto endpoint{ boost::asio::connect(socket, targets) }; //< connect() throws on error

					// TODO: write log message; successfully connected to endpointe
				} catch (boost::system::system_error const& ex) {
					throw make_exception("Failed to connect to \"", host, ':', port, "\". Original exception: ", ex.what());
				}


				// authenticate with the server
				if (!authenticate(password)) {
					// throw authentication failure exception
					throw make_exception("Authentication failed!");
				}

				// TODO: write log message; successfully authenticated with server
			}
			~RconClient()
			{
				ioContext.run(); //< wait for async operations to finish
				socket.close(); //< close the socket
			}

			std::string command(std::string const& command)
			{
				const auto packetId{ get_next_packet_id() };
				const buffer packet{ build_packet(packet_header{ get_packet_size(command.size()), packetId, (int32_t)PacketType::SERVERDATA_EXECCOMMAND }, command) };

				if (boost::asio::write(socket, boost::asio::buffer(packet)) != packet.size()) {
					// TODO: write log message; failed to send packet
					throw make_exception("Failed to send packet for command \"", command, "\"!");
				}

				const auto termPacketId{ get_next_packet_id() };
				const buffer termPacket{ build_terminator_packet(termPacketId) };

				if (boost::asio::write(socket, boost::asio::buffer(termPacket)) != termPacket.size()) {
					// TODO: write log message; failed to send terminator packet
					throw make_exception("Failed to send terminator packet for command \"", command, "\"!");
				}

				// receive all of the packets & concatenate them
				std::stringstream ss;

				for (std::pair<packet_header, buffer> response{ recv() }; response.first.id != termPacketId; response = recv()) {
					ss << bytes_to_string(response.second);
				}

				return ss.str();
			}

			bool authenticate(std::string_view password)
			{
				const buffer p{ build_packet(packet_header{ get_packet_size(password.size()), 1, (int32_t)PacketType::SERVERDATA_AUTH }, password.data()) };

				if (boost::asio::write(socket, boost::asio::buffer(p)) != p.size()) {
					// TODO: write log message; failed to send authentication packet
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

int main(const int argc, char** argv)
{
	try {
		const opt3::ArgManager args{ argc, argv,
			// define capturing args:
			opt3::make_template(opt3::CaptureStyle::Required, opt3::ConflictStyle::Conflict, 'H', "host", "hostname"),
			opt3::make_template(opt3::CaptureStyle::Required, opt3::ConflictStyle::Conflict, 'S', "saved"),
			opt3::make_template(opt3::CaptureStyle::Required, opt3::ConflictStyle::Conflict, 'P', "port"),
			opt3::make_template(opt3::CaptureStyle::Required, opt3::ConflictStyle::Conflict, 'p', "pass", "password"),
			opt3::make_template(opt3::CaptureStyle::Required, opt3::ConflictStyle::Conflict, 'w', "wait"),
			opt3::make_template(opt3::CaptureStyle::Required, opt3::ConflictStyle::Conflict, 'f', "file"),
			opt3::make_template(opt3::CaptureStyle::Required, opt3::ConflictStyle::Conflict, "save-host"),
			opt3::make_template(opt3::CaptureStyle::Required, opt3::ConflictStyle::Conflict, "remove-host"),
		};

		const auto host{ args.getv_any<opt3::Flag, opt3::Option>('H', "host", "hostname").value() };
		const auto port{ args.getv_any<opt3::Flag, opt3::Option>('P', "port").value() };
		const auto pass{ args.getv_any<opt3::Flag, opt3::Option>('p', "pass", "password").value() };

		net::rcon::RconClient client{ host, port, pass };

		std::cout
			<< client.command("listgamemodeproperties") << std::endl
			;

		return 0;
	} catch (std::exception const& ex) {
		std::cerr << csync.get_fatal() << ex.what() << std::endl;
		return 1;
	} catch (...) {
		std::cerr << csync.get_fatal() << "An undefined exception occurred!" << std::endl;
		return 1;
	}
}
