// Fill out your copyright notice in the Description page of Project Settings.


#include "StockfishManager.h"
#include "Misc/Paths.h"
#include "HAL/PlatformProcess.h"

UStockfishManager::UStockfishManager()
{
    ReadPipe = nullptr;
    WritePipe = nullptr;
}

UStockfishManager::~UStockfishManager()
{
    StopEngine();
}

void UStockfishManager::StartEngine()
{
    FString StockfishPath = FPaths::Combine(FPaths::ProjectDir(), TEXT("Binaries/Win64/stockfish.exe"));

    if (!FPaths::FileExists(StockfishPath))
    {
        UE_LOG(LogTemp, Error, TEXT("Stockfish executable not found at %s"), *StockfishPath);
        return;
    }

    FPlatformProcess::CreatePipe(ReadPipe, WritePipe);
    ProcessHandle = FPlatformProcess::CreateProc(*StockfishPath, nullptr, false, true, true, nullptr, 0, nullptr, WritePipe, ReadPipe);

    if (!ProcessHandle.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to start Stockfish process."));
    }
}

void UStockfishManager::StopEngine()
{
    if (ProcessHandle.IsValid())
    {
        FPlatformProcess::TerminateProc(ProcessHandle);
        FPlatformProcess::CloseProc(ProcessHandle);
    }

    if (ReadPipe)
    {
        FPlatformProcess::ClosePipe(0, ReadPipe);
        ReadPipe = nullptr;
    }
    if (WritePipe)
    {
        FPlatformProcess::ClosePipe(0, WritePipe);
        WritePipe = nullptr;
    }
}

FString UStockfishManager::GetBestMove(const FString& FEN)
{
    if (!ProcessHandle.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("StockfishManager: Cannot get best move, process is not valid."));
        return TEXT("");
    }

    FString Command = FString::Printf(TEXT("position fen %s\ngo movetime 1000\n"), *FEN);
    UE_LOG(LogTemp, Log, TEXT("StockfishManager: Sending command to Stockfish: %s"), *Command.Replace(TEXT("\n"), TEXT(" ")));

    FPlatformProcess::WritePipe(WritePipe, (uint8*)TCHAR_TO_ANSI(*Command), Command.Len());

    FPlatformProcess::Sleep(1.1f); // Wait for stockfish to think

    FString Output = FPlatformProcess::ReadPipe(ReadPipe);
    UE_LOG(LogTemp, Log, TEXT("StockfishManager: Received output:\n%s"), *Output);
    
    TArray<FString> Lines;
    Output.ParseIntoArray(Lines, TEXT("\n"), true);

    for (const FString& Line : Lines)
    {
        if (Line.StartsWith(TEXT("bestmove")))
        {
            TArray<FString> Parts;
            Line.ParseIntoArray(Parts, TEXT(" "), true);
            if (Parts.Num() > 1)
            {
                UE_LOG(LogTemp, Log, TEXT("StockfishManager: Parsed best move: %s"), *Parts[1]);
                return Parts[1];
            }
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("StockfishManager: Could not find 'bestmove' in Stockfish output."));
    return TEXT("");
}
