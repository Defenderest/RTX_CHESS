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
		string PostgreSqlPath = "C:/Program Files/PostgreSQL/16";

		if (Directory.Exists(PostgreSqlPath))
        {
            PublicIncludePaths.Add(Path.Combine(PostgreSqlPath, "include"));
            PublicLibraryPaths.Add(Path.Combine(PostgreSqlPath, "lib"));
            PublicAdditionalLibraries.Add("libpq.lib");

			// Копируем DLL в директорию сборки, чтобы игра могла их найти при запуске.
            RuntimeDependencies.Add(Path.Combine(PostgreSqlPath, "bin", "libpq.dll"));
			RuntimeDependencies.Add(Path.Combine(PostgreSqlPath, "bin", "libcrypto-3-x64.dll")); // Зависимость libpq
			RuntimeDependencies.Add(Path.Combine(PostgreSqlPath, "bin", "libssl-3-x64.dll"));    // Зависимость libpq
        }
        else
        {
            // Выводим ошибку, если путь не найден, чтобы пользователь знал, что нужно исправить.
            System.Console.WriteLine("!!! PostgreSQL path not found. Please edit RTX_CHESS.Build.cs and set the correct path to your PostgreSQL installation. !!!");
        }
	}
}
