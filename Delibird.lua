workspace "Delibird"
    configurations {"Debug", "Release"}
    architecture "x86"

    -- Localizaciones Comunes --
    ----------------------------
    CommonLocation = "%{wks.location}/%{prj.name}"
    CommonTargetDir = "%{wks.location}/%{prj.name}/%{cfg.buildcfg}"
    CommonObjDir = "%{wks.location}/%{prj.name}/obj/%{cfg.buildcfg}"

    -- Opciones Comunes para Debug --
    ---------------------------------
    CommonSimbols = "On"
    CommonDebugWarnings = "Extra"

    CommonDebugDefines = "DB_DEBUG"

    -- Opciones Comunes para Release --
    ---------------------------------
    CommonOptimize = "Speed"
    CommonReleaseWarnings = "Default"
    CommonReleaseDefines = "DB_RELEASE"

    include ("SharedLibrary/SharedLibrary.lua")
    include ("Team/Team.lua")
    include ("Broker/Broker.lua")
    include ("gameboy/gameboy.lua")
    include ("GameCard/GameCard.lua")
