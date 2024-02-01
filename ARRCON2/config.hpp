#pragma once
// 307lib
#include <env.hpp>

// STL
#include <filesystem>	//< for std::filesystem::path
#include <simpleINI.hpp>//< for ini::INI

namespace config {
	inline constexpr const auto HEADER_APPERANCE{ "appearance" };
	inline constexpr const auto HEADER_TIMING{ "timing" };
	inline constexpr const auto HEADER_TARGET{ "target" };
	inline constexpr const auto HEADER_MISC{ "miscellaneous" };

	/**
	 * @class	Locator
	 * @brief	Used to locate ARRCON's config files.
	 */
	class Locator {
		std::filesystem::path program_location;
		std::string name_no_ext;
		std::filesystem::path env_path;
		std::filesystem::path home_path;

	public:
		Locator(const std::filesystem::path& program_dir, const std::string& program_name_no_extension) : program_location{ program_dir }, name_no_ext{ program_name_no_extension }, env_path{ env::getvar(name_no_ext + "_CONFIG_DIR").value_or("") }, home_path{ env::get_home() } {}

		/**
		 * @brief		Retrieves the target location of the given file extension appended to the program name. (Excluding extension, if applicable.)
		 * @param ext	The file extension of the target file.
		 * @returns		std::filesystem::path
		 *\n			This is NOT guaranteed to exist! If no valid config file was found, the .config directory in the user's home directory is returned.
		 */
		std::filesystem::path from_extension(const std::string& ext) const
		{
			if (ext.empty())
				throw make_exception("Empty extension passed to Locator::from_extension()!");
			std::string target{ name_no_ext + ((ext.front() != '.') ? ("." + ext) : ext) };
			std::filesystem::path path;
			// 1:  check the environment
			if (!env_path.empty()) {
				path = env_path / target;
				return path;
			}
			// 2:  check the program directory. (support portable versions by checking this before the user's home dir)
			if (path = program_location / target; std::filesystem::exists(path))
				return path;
			// 3:  user's home directory:
			path = home_path / ".config" / name_no_ext / target;
			return path; // return even if it doesn't exist
		}
	};
}
