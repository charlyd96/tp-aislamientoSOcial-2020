project "SharedLibrary"
    kind "SharedLib"
    language "C"
    targetname "SharedLibrary"

    location (CommonLocation)
    targetdir (CommonTargetDir)
    objdir (CommonObjDir)

    includedirs{
        "%{prj.location}/"
    }

    files{
        "%{prj.location}/**.h",
        "%{prj.location}/**.c"
    }

    filter "configurations:Debug"
        symbols "On"
        warnings "Extra"
      
        defines{
            "DB_DEBUG"
        }

    filter "configurations:Release"
        optimize "Speed"
        warnings "Default"
        defines{
            "DB_RELEASE"
        }