solution "opc"
	location "build"
	targetdir "bin"
	objdir "build/obj"
	configurations { "Debug", "Release" }

	project "opc"
		kind "ConsoleApp"
		language "C++"
		files { "src/**.h", "src/**.cpp" }

		includedirs {
			"/usr/local/include",
		}
		libdirs {
			"/usr/local/lib",
		}
		buildoptions { "-x objective-c++ -lc++ -std=c++11 -stdlib=libc++" }

		links {
			"avutil",
			"avformat",
			"avcodec",
			"swscale",
			"avfilter",
			"swresample",
			"bz2",
			"z",
			"portaudio",
			"ncurses"
		}

		configuration "Debug"
			defines { "DEBUG" }
			flags { "Symbols" }
		configuration "Release"
			defines { "NDEBUG" }
			flags { "Optimize" }
