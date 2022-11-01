/**
 * @file	HostInfo.hpp
 * @author	radj307
 * @brief	Contains the HostInfo struct, an object used to store a target's connection information.
 */
#pragma once
#include <INIRedux.hpp>

#include <string>
#include <optional>

namespace net {
	/**
	 * @struct	HostInfo
	 * @brief	Contains the connection info for a single target.
	 */
	struct HostInfo {
		std::string hostname, port, password;

		HostInfo() = default;
		HostInfo(const std::string& hostname, const std::string& port, const std::string& password) : hostname{ hostname }, port{ port }, password{ password } {}
		HostInfo(const file::INI::SectionContent& ini_section, const HostInfo& default_target)
		{
			// hostname:
			if (const auto host{ ini_section.find("sHost") }; host != ini_section.end())
				hostname = file::ini::to_string(host->second);
			else hostname = default_target.hostname;
			// port:
			if (const auto prt{ ini_section.find("sPort") }; prt != ini_section.end())
				port = file::ini::to_string(prt->second);
			else port = default_target.port;
			// password:
			if (const auto pass{ ini_section.find("sPass") }; pass != ini_section.end())
				password = file::ini::to_string(pass->second);
			else password = default_target.password;
		}
		HostInfo(const file::INI::SectionContent& ini_section) : HostInfo(ini_section, HostInfo()) {}

		/**
		 * @brief			Create a HostInfo struct containing values from the given optional overrides, or values from this HostInfo instance for any null overrides.
		 * @param ohost		Optional Override Hostname
		 * @param oport		Optional Override Port
		 * @param opass		Optional Override Password
		 * @returns			HostInfo
		 */
		HostInfo copyWithOverrides(const std::optional<std::string>& ohost, const std::optional<std::string>& oport, const std::optional<std::string>& opass) const
		{
			return{ ohost.value_or(hostname), oport.value_or(port), opass.value_or(password) };
		}
		/**
		 * @brief			Create a HostInfo struct containing values from the given optional overrides, or values from this HostInfo instance for any null overrides.
		 * @param ohost		Optional Override Hostname
		 * @param oport		Optional Override Port
		 * @param opass		Optional Override Password
		 * @returns			HostInfo&&
		 */
		HostInfo&& moveWithOverrides(const std::optional<std::string>& ohost, const std::optional<std::string>& oport, const std::optional<std::string>& opass)
		{
			if (ohost.has_value())
				hostname = ohost.value();
			if (oport.has_value())
				port = oport.value();
			if (opass.has_value())
				password = opass.value();
			return std::move(*this);
		}

		operator file::INI::SectionContent() const
		{
			file::INI::SectionContent section;

			section.insert_or_assign("sHost", hostname);
			section.insert_or_assign("sPort", port);
			section.insert_or_assign("sPass", password);

			return section;
		}

		friend std::ostream& operator<<(std::ostream& os, const HostInfo& hostinfo)
		{
			return (os
				<< "sHost = " << hostinfo.hostname << '\n'
				<< "sPort = " << hostinfo.port << '\n'
				<< "sPass = " << hostinfo.password << '\n'
				).flush();
		}
		bool operator==(const HostInfo& o) const { return hostname == o.hostname && port == o.port && password == o.password; }
		bool operator!=(auto&& o) const { return !operator==(std::forward<decltype(o)>(o)); }
	};

	// for more descriptive names
	using HostList = file::INI;
}