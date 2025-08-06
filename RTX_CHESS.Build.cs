// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class RTX_CHESS : ModuleRules
{
	public RTX_CHESS(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "Projects", "UMG", "Http", "Json", "JsonUtilities", "OnlineSubsystem", "Sockets", "Networking", "Niagara" });

		// Uncomment if you are using Slate UI
		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore", "RenderCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true

		// --- PostgreSQL Integration ---
		// !!ВАЖНО!! Замените этот путь на путь к вашей установке PostgreSQL
		string PostgreSqlPath = "D:\\postgreSQL";

		if (Directory.Exists(PostgreSqlPath))
        {
            string includePath = Path.Combine(PostgreSqlPath, "include");
            string libPath = Path.Combine(PostgreSqlPath, "lib");
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

			// Копируем DLL в директорию сборки, чтобы игра могла их найти при запуске.
            string binPath = Path.Combine(PostgreSqlPath, "bin");
            // Добавляем все потенциальные зависимости libpq.dll, чтобы избежать ошибок загрузки модуля.
            // Новые версии PostgreSQL могут также требовать библиотеки для сжатия (lz4, zstd), xml и kerberos.
            string[] DllsToCopy = {
                "libpq.dll", "libcrypto-3-x64.dll", "libssl-3-x64.dll", "libintl-9.dll",
                "libiconv-2.dll", "libwinpthread-1.dll", "zlib1.dll", "liblz4.dll", "libzstd.dll",
                "libxml2.dll", "krb5_64.dll"
            };
            foreach(string DllName in DllsToCopy)
            {
                string DllPath = Path.Combine(binPath, DllName);
                if (!File.Exists(DllPath))
                {
                    // Эта проверка прервет сборку и точно укажет, какой DLL не хватает в вашей установке PostgreSQL 17.
                    throw new BuildException($"PostgreSQL 17 dependency check: DLL '{DllName}' was not found in '{binPath}'. Please verify your installation or update the list in RTX_CHESS.Build.cs.");
                }
                RuntimeDependencies.Add(DllPath);
            }
        }
        else
        {
            // Выводим ошибку, если путь не найден, чтобы пользователь знал, что нужно исправить.
            throw new BuildException($"PostgreSQL path not found at '{PostgreSqlPath}'. Please edit RTX_CHESS.Build.cs and set the correct path to your PostgreSQL installation.");
        }
	}
}
