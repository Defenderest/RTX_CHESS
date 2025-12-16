// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class RTX_CHESS : ModuleRules
{
    public RTX_CHESS(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PrivateIncludePaths.Add(ModuleDirectory);

        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "UMG", "Http", "Json", "JsonUtilities", "OnlineSubsystem", "Sockets", "Networking", "Niagara" });

        // Uncomment if you are using Slate UI
        PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

       

        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");

        // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true

        // --- PostgreSQL Integration (Temporarily Disabled) ---
        /*
		// !!IMPORTANT!! Replace this path with the path to your PostgreSQL installation
		string PostgreSqlPath = "D:\\postgreSQL";

		if (Directory.Exists(PostgreSqlPath))
        {
            string includePath = Path.Combine(PostgreSqlPath, "include");
            string libPath = Path.Combine(PostgreSqlPath, "lib");
            string binPath = Path.Combine(PostgreSqlPath, "bin");

            string headerFile = Path.Combine(includePath, "libpq-fe.h");
            string libFile = Path.Combine(libPath, "libpq.lib");

            if (!File.Exists(headerFile))
            {
                throw new BuildException($"PostgreSQL header file 'libpq-fe.h' not found in '{includePath}'. Please check your PostgreSQL installation and the path in RTX_CHESS.Build.cs. You might need to install the development headers (libpq).");
            }
            if (!File.Exists(libFile))
            {
                throw new BuildException($"PostgreSQL library file 'libpq.lib' not found in '{libPath}'. Please check your PostgreSQL installation and the path in RTX_CHESS.Build.cs. You might need to install the development headers (libpq).");
            }

            PublicIncludePaths.Add(includePath);
            PublicAdditionalLibraries.Add(libFile);

			// Copy DLLs to the build directory. The previous approach was too strict and caused build errors
            // if the PostgreSQL version or its components differed.
            // Now we only make libpq.dll mandatory, and all others optional.
            // This should fix the build error. If a runtime error related to DLL loading occurs later,
            // we will need to find the exact list of required files for your PostgreSQL version.
            string[] RequiredDlls = {
                "libpq.dll" // This is the only library that is guaranteed to be needed.
            };

            string[] OptionalDlls = {
                // OpenSSL dependencies (names may vary depending on version)
                "libcrypto-3-x64.dll", "libssl-3-x64.dll",
                // ICU dependencies (for internationalization, names depend on version)
                "icudt67.dll", "icuun67.dll", "icuio67.dll", "icutu67.dll", "icuuc67.dll",
                // Other possible dependencies
                "libintl-9.dll", "libiconv-2.dll", "libwinpthread-1.dll", "libpgtypes.dll",
                "libcurl.dll", "libecpg.dll", "libecpg_compat.dll", "libxml2.dll", "libxslt.dll",
                "zlib1.dll", "liblz4.dll", "libzstd.dll", "krb5_64.dll"
            };

            foreach(string DllName in RequiredDlls)
            {
                string DllPath = Path.Combine(binPath, DllName);
                if (!File.Exists(DllPath))
                {
                    // This check will abort the build and indicate exactly which DLL is missing.
                    throw new BuildException($"PostgreSQL dependency check: Required DLL '{DllName}' was not found in '{binPath}'. Please verify your PostgreSQL installation is complete and the path in RTX_CHESS.Build.cs is correct.");
                }
                RuntimeDependencies.Add(DllPath);
            }

            foreach(string DllName in OptionalDlls)
            {
                string DllPath = Path.Combine(binPath, DllName);
                if (File.Exists(DllPath))
                {
                    RuntimeDependencies.Add(DllPath);
                }
                else
                {
                    // Output a warning to the build log, but do not abort it.
                    System.Console.WriteLine($"Warning: PostgreSQL dependency check: Optional DLL '{DllName}' was not found in '{binPath}'. This may be fine if your installation doesn't include it or you don't use features that depend on it.");
                }
            }
        }
        else
        {
            // Output an error if the path is not found, so the user knows what needs to be fixed.
            throw new BuildException($"PostgreSQL path not found at '{PostgreSqlPath}'. Please edit RTX_CHESS.Build.cs and set the correct path to your PostgreSQL installation.");
        }
		*/
    }
}
