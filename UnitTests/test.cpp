#include <sysarch.h>
// Include Catch2
#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>

// Include Subject Files
#include "../ARRCON/network/packet.hpp"
using namespace packet;

#define REQUIRE_THROW REQUIRE_THROWS
#define CHECK_THROW CHECK_THROWS

TEST_CASE("Packet Type Testing", "[PACKET]")
{
	REQUIRE(Type::SERVERDATA_AUTH == 3);
	REQUIRE(Type::SERVERDATA_AUTH_RESPONSE == 2);
	REQUIRE(Type::SERVERDATA_EXECCOMMAND == 2);
	REQUIRE(Type::SERVERDATA_RESPONSE_VALUE == 0);
}

TEST_CASE("Packet Testing", "[PACKET]")
{
	Packet p;

	SECTION("Packet Boundary Testing") {
		std::string longstr(PSIZE_MAX_SEND + 1, '@');
		REQUIRE(p.isValidLength()); // ensure the packet reports a valid length
		REQUIRE(!p.isValid());
		p = { PID_MIN, Type::SERVERDATA_EXECCOMMAND, longstr };
		REQUIRE(!p.isValidLength());
		REQUIRE(p.isValid());
	}

	SECTION("Packet Constructor Testing") {
		p = { PID_MIN, Type::SERVERDATA_AUTH, "password" };
		REQUIRE(p.body == "password");
		REQUIRE(p.size == static_cast<size_t>((sizeof(int) * 2) + p.body.size() + 2));
		REQUIRE(p.isValid());
		REQUIRE(p.isValidLength());
		std::string body{ "Hello World!" };
	}

	SECTION("Serialized Packet Testing") {
		// check to make sure serialized packets aren't corrupted when converted from Packets.
		const auto sp{ p.serialize() };
		REQUIRE(sp.size == p.size);
		REQUIRE(sp.id == p.id);
		REQUIRE(sp.body == p.body);
		REQUIRE(sp.type == p.type);
		// check to make sure packets aren't corrupted when converted from serialized packets.
		p = { serialized_packet(0, 0, 0, {0x00}) };
		REQUIRE(p.body.size() == 0ull);
		REQUIRE(p.body == "");
		REQUIRE(!p.isValid());
		REQUIRE(p.isValidLength());
		REQUIRE(p.type == 0);
		REQUIRE(p.size == 0);
		REQUIRE(p.id == 0);
	}

	SECTION("Zeroed Packet Testing") {
		p = { PID_MIN, Type::SERVERDATA_AUTH, "this will be removed anyway" };
		REQUIRE(p.size != 0);
		REQUIRE(p.type != 0);
		REQUIRE(!p.body.empty());
		p.zero();
		REQUIRE(p.size == 0);
		REQUIRE(p.id == 0);
		REQUIRE(p.type == 0);
		REQUIRE(p.body.empty());
	}
}


#include "../ARRCON/network/net.hpp"

TEST_CASE("Network functions test", "[RCON]")
{
	SECTION("Error Reporting") {
		CHECK(LAST_SOCKET_ERROR_CODE() == 0);
		const auto msg_comp{ net::getLastSocketErrorMessage().compare("The operation completed successfully.") };
		const int error_margin{ 2 }; ///< Maximum number of non-matching characters when last socket error message is compared to the above string
		CHECK(msg_comp >= -error_margin);
		CHECK(msg_comp <= error_margin);
	}

	SECTION("winsock testing (Windows-Only)") {
		#ifdef OS_WIN
		REQUIRE_NOTHROW(net::init());		// winsock initialization
		REQUIRE_NOTHROW(net::cleanup());	// interrupt handler
		// windows doesn't consider sockets & file descriptors to be the same thing
		CHECK_NOTHROW(net::close_socket(0)); // FD: STDIN
		CHECK_NOTHROW(net::close_socket(1)); // FD: STDOUT
		CHECK_NOTHROW(net::close_socket(2)); // FD: STDERR
		#endif
	}

	SECTION("Connect") {
		SOCKET sd;
		REQUIRE_THROW(sd = net::connect("localhost", "-1"));
		REQUIRE(net::isValidSocket(sd));
	}
}

#include "../ARRCON/network/rcon.hpp"


TEST_CASE("Rcon functions test", "[RCON]")
{

}

