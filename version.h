/**
 * @file	version.h
 * @author	autogenerated by radj307
 */
#pragma once
#define ARRCON_VERSION_MAJOR	3.2.6
#define ARRCON_VERSION_MINOR	3.2.6
#define ARRCON_VERSION_PATCH	3.2.6

// Stringize function
#define VERSION_H_STRINGIZEX(x)  #x
#define VERSION_H_STRINGIZE(x)   VERSION_H_STRINGIZEX(x)

#define ARRCON_VERSION VERSION_H_STRINGIZE(ARRCON_VERSION_MAJOR) "." VERSION_H_STRINGIZE(ARRCON_VERSION_MINOR) "." VERSION_H_STRINGIZE(ARRCON_VERSION_PATCH)