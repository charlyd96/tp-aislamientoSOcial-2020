project "GameBoy"

    kind "ConsoleApp"
    language "C"
    
    location  (CommonLocation)
    targetdir (CommonTargetDir)
    objdir    (CommonObjDir)

    includedirs{
        "%{prj.location}/",
        "%{wks.location}/SharedLibrary/"
    }

    files{
        "%{prj.location}/**.c"
    }

    links{
        "SharedLibrary",
        "commons",
        "pthread"
    }

    filter "configurations:Debug"
        symbols       (CommonSimbols)
        warnings      (CommonDebugWarnings)
        fatalwarnings (CommonDebugFatalWarnins)
        defines       (CommonDebugDefines)

    filter "configurations:Release"
        optimize (CommonOptimize)
        warnings (CommonReleaseWarnings)
        defines  (CommonReleaseDefines)