#pragma once
#include "logging.hpp"
#include "net/target_info.hpp"

// 307lib::filelib
#include <simpleINI.hpp>	//< for ini::INI

// STL
#include <filesystem>		//< for std::filesystem::path
#include <map>				//< for std::map

namespace config {
	inline constexpr const auto HEADER_APPERANCE{ "appearance" };
	inline constexpr const auto HEADER_TARGET{ "target" };
	inline constexpr const auto HEADER_MISC{ "miscellaneous" };

	class SavedHosts {
		using target_info = net::rcon::target_info;
		using map = std::map<std::string, target_info>;

		map hosts;

	public:
		SavedHosts() = default;
		SavedHosts(ini::INI const& ini)
		{
			import_from(ini);
		}
		SavedHosts(std::filesystem::path const& path) : SavedHosts(ini::INI(path)) {}

		auto begin() const { return hosts.begin(); }
		auto end() const { return hosts.end(); }
		bool empty() const noexcept { return hosts.empty(); }
		size_t size() const noexcept { return hosts.size(); }
		bool contains(std::string const& name) const { return hosts.contains(name); }

		void import_from(ini::INI const& ini)
		{
			if (ini.contains("")) {
				// warn about global keys
				const auto globalKeysCount{ ini.at("").size() };
				std::clog << MessageHeader(LogLevel::Warning) << "Hosts file contains " << globalKeysCount << " key" << (globalKeysCount == 1 ? "" : "s") << " that aren't associated with a saved host!" << std::endl;
			}

			// enumerate entries
			for (const auto& [entryKey, entryContent] : ini) {
				// enumerate key-value pairs
				for (const auto& [key, value] : entryContent) {
					const std::string keyLower{ str::tolower(key) };

					if (str::equalsAny<false>(keyLower, "sHost")) {
						hosts[entryKey].host = value;

						std::clog << MessageHeader(LogLevel::Trace) << '[' << entryKey << ']' << " Imported hostname \"" << value << '\"' << std::endl;
					}
					else if (str::equalsAny<false>(keyLower, "sPort")) {
						hosts[entryKey].port = value;

						std::clog << MessageHeader(LogLevel::Trace) << '[' << entryKey << ']' << " Imported port \"" << value << '\"' << std::endl;
					}
					else if (str::equalsAny<false>(keyLower, "sPass")) {
						hosts[entryKey].pass = value;

						std::clog << MessageHeader(LogLevel::Trace) << '[' << entryKey << ']' << " Imported password \"" << std::string(value.size(), '*') << '\"' << std::endl;
					}
					else {
						std::clog << MessageHeader(LogLevel::Warning) << '[' << entryKey << ']' << " Skipped unrecognized key \"" << key << "\"" << std::endl;
					}
				}
			}
		}
		void export_to(ini::INI& ini) const
		{
			for (const auto& [name, info] : hosts) {
				ini[name] = ini::Section{
					std::make_pair("sHost", info.host),
					std::make_pair("sPort", info.port),
					std::make_pair("sPass", info.pass),
				};

				std::clog << MessageHeader(LogLevel::Trace) << '[' << name << ']' << " was exported successfully." << std::endl;
			}
		}

		std::optional<target_info> get_host(std::string const& name) const
		{
			if (const auto& it{ hosts.find(name) }; it != hosts.end()) {
				return it->second;
			}
			else return std::nullopt;
		}

		auto& operator[](std::string const& name)
		{
			return hosts[name];
		}
	};
}
