// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class RTX_CHESS : ModuleRules
{
    public RTX_CHESS(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "UMG", "Http", "Json", "JsonUtilities", "OnlineSubsystem", "Sockets", "Networking", "Niagara" });

        // Uncomment if you are using Slate UI
        PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

       

        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");

        // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true

        // --- PostgreSQL Integration (Temporarily Disabled) ---
        /*
		// !!ВАЖНО!! Замените этот путь на путь к вашей установке PostgreSQL
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

			// Копируем DLL в директорию сборки. Предыдущий подход был слишком строгим и приводил к ошибкам
            // сборки, если версия PostgreSQL или ее компоненты отличались.
            // Теперь мы делаем обязательной только libpq.dll, а все остальные -- опциональными.
            // Это должно исправить ошибку сборки. Если после этого возникнет ошибка во время запуска
            // игры, связанная с загрузкой DLL, нам нужно будет найти точный список необходимых файлов
            // для вашей версии PostgreSQL.
            string[] RequiredDlls = {
                "libpq.dll" // Это единственная библиотека, которая гарантированно нужна.
            };

            string[] OptionalDlls = {
                // Зависимости OpenSSL (названия могут меняться в зависимости от версии)
                "libcrypto-3-x64.dll", "libssl-3-x64.dll",
                // Зависимости ICU (для интернационализации, названия зависят от версии)
                "icudt67.dll", "icuun67.dll", "icuio67.dll", "icutu67.dll", "icuuc67.dll",
                // Другие возможные зависимости
                "libintl-9.dll", "libiconv-2.dll", "libwinpthread-1.dll", "libpgtypes.dll",
                "libcurl.dll", "libecpg.dll", "libecpg_compat.dll", "libxml2.dll", "libxslt.dll",
                "zlib1.dll", "liblz4.dll", "libzstd.dll", "krb5_64.dll"
            };

            foreach(string DllName in RequiredDlls)
            {
                string DllPath = Path.Combine(binPath, DllName);
                if (!File.Exists(DllPath))
                {
                    // Эта проверка прервет сборку и точно укажет, какой DLL не хватает.
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
                    // Выводим предупреждение в лог сборки, но не прерываем ее.
                    System.Console.WriteLine($"Warning: PostgreSQL dependency check: Optional DLL '{DllName}' was not found in '{binPath}'. This may be fine if your installation doesn't include it or you don't use features that depend on it.");
                }
            }
        }
        else
        {
            // Выводим ошибку, если путь не найден, чтобы пользователь знал, что нужно исправить.
            throw new BuildException($"PostgreSQL path not found at '{PostgreSqlPath}'. Please edit RTX_CHESS.Build.cs and set the correct path to your PostgreSQL installation.");
        }
		*/
    }
}
