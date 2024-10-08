/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2024 Alexander Samusev.

This file is part of the Hork Engine Source Code.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#include <Hork/Core/CoreApplication.h>

#include <Hork/Core/IO.h>
#include <Hork/Core/Parse.h>
#include <Hork/Core/Logger.h>
#include <Hork/Core/Platform.h>
#include <Hork/MaterialGraph/MaterialCompiler.h>
#include <Hork/MaterialGraph/MaterialGraph.h>
#include <Hork/ShaderUtils/ShaderCompiler.h>
#include <Hork/Resources/Resource_Material.h>

HK_NAMESPACE_BEGIN

bool CompileMaterial(StringView input, StringView output, bool debugMode)
{
    LOG("Loading {}\n", input);
    auto file = File::sOpenRead(input);
    if (!file)
    {
        LOG("Failed to open material graph {}\n", input);
        return false;
    }

    auto graph = MaterialGraph::sLoad(file);
    if (!graph)
    {
        LOG("Failed to load material graph {}\n", input);
        return false;
    }

    LOG("Compiling {}\n", input);
    MaterialResourceBuilder builder;
    auto material = builder.Build(*graph, debugMode);
    if (!material)
    {
        LOG("Failed to build material graph {}\n", input);
        return false;
    }

    LOG("Write {}\n", output);
    auto outfile = File::sOpenWrite(output);
    if (!outfile)
    {
        LOG("Failed to write compiled material {}\n", output);
        return false;
    }

    material->Write(outfile);
    return true;
}

int RunApplication()
{
    Core::SetEnableConsoleOutput(true);

    const char* help = R"(
    -h                      -- Help
    -s <filename>           -- Source filename (material graph)
    -o <filename>           -- Output filename
    -debug                  -- Compile material in debug mode
    )";

    auto& args = CoreApplication::sArgs();
    int i;

    const char* inputFile = nullptr;
    const char* outputFile = nullptr;

    i = args.Find("-h");
    if (i != -1)
    {
        LOG(help);
        return 0;
    }

    i = args.Find("-s");
    if (i == -1 || i + 1 >= args.Count())
    {
        LOG("Source file is not specified. Use -s <filename>\n");
        return -1;
    }

    inputFile = args.At(i + 1);

    i = args.Find("-o");
    if (i == -1 || i + 1 >= args.Count())
    {
        LOG("Output file is not specified. Use -o <filename>\n");
        return -1;
    }

    outputFile = args.At(i + 1);

    bool debugMode = args.Find("-debug") != -1;

    ShaderCompiler::sInitialize();

    bool result = CompileMaterial(inputFile, outputFile, debugMode);

    ShaderCompiler::sDeinitialize();

    return result ? 0 : -1;
}

HK_NAMESPACE_END

using ApplicationClass = Hk::CoreApplication;

alignas(alignof(ApplicationClass)) static char AppData[sizeof(ApplicationClass)];

int main(int argc, char* argv[])
{
    using namespace Hk;

#if defined(HK_DEBUG) && defined(HK_COMPILER_MSVC)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

#ifdef HK_OS_WIN32
    ArgumentPack args;
#else
    ArgumentPack args(argc, argv);
#endif

    ApplicationClass* app = new (AppData) ApplicationClass(args);
    int exitCode = RunApplication();
    app->~ApplicationClass();
    return exitCode;
}
