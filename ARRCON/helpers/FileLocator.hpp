#pragma once
// 307lib::shared
#include <make_exception.hpp>	//< for ex::make_exception
#include <env.hpp>				//< for env::getvar

// STL
#include <filesystem>			//< for std::filesystem

/**
 * @class	FileLocator
 * @brief	Used to locate ARRCON's config files.
 */
class FileLocator {
	std::filesystem::path program_location;
	std::string name_no_ext;
	std::filesystem::path env_path;
	std::filesystem::path home_path;

public:
	FileLocator(std::filesystem::path const& program_dir, std::string const& program_name_no_extension) :
		program_location{ program_dir },
		name_no_ext{ program_name_no_extension },
		env_path{ env::getvar(name_no_ext + "_CONFIG_DIR").value_or("") },
		home_path{ env::get_home() }
	{
	}
	FileLocator(std::filesystem::path const& program_dir, std::filesystem::path const& program_name_no_extension) :
		FileLocator(program_dir, program_name_no_extension.generic_string())
	{
	}

	/**
	 * @brief		Retrieves the target location of the given file extension appended to the program name. (Excluding extension, if applicable.)
	 * @param ext	The file extension of the target file.
	 * @returns		std::filesystem::path
	 *\n			This is NOT guaranteed to exist! If no valid config file was found, the .config directory in the user's home directory is returned.
	 */
	std::filesystem::path from_extension(std::string const& ext) const
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
