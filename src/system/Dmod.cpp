#include "Dmod.hpp"

Dmod_Context_t* Dmod::LoadFile(const char* Path)
{
    return Dmod_LoadFile(Path);
}

Dmod_Context_t* Dmod::Load(const void* Data, size_t Size)
{
    return Dmod_Load(Data, Size);
}

bool Dmod::Unload(Dmod_Context_t* Context, bool Force)
{
    return Dmod_Unload(Context, Force);
}

bool Dmod::Initialize(size_t NumIrqs, size_t MaxHandlersPerIrq)
{
    return Dmod_Initialize(NumIrqs, MaxHandlersPerIrq);
}

bool Dmod::ConnectApi(Dmod_Api_t* OutputsApi, Dmod_Api_t* InputsApi)
{
    return Dmod_ConnectApi(OutputsApi, InputsApi);
}

bool Dmod::DisconnectApi(Dmod_Api_t* OutputsApi, Dmod_Api_t* InputsApi)
{
    return Dmod_DisconnectApi(OutputsApi, InputsApi);
}

bool Dmod::ConnectOutputApis(Dmod_Context_t* Context)
{
    return Dmod_ConnectOutputApis(Context);
}

bool Dmod::ConnectInputApis(Dmod_Context_t* Context)
{
    return Dmod_ConnectInputApis(Context);
}

bool Dmod::ConnectAllApis(Dmod_Context_t* Context)
{
    return Dmod_ConnectAllApis(Context);
}

bool Dmod::DisconnectOutputApis(Dmod_Context_t* Context)
{
    return Dmod_DisconnectOutputApis(Context);
}

bool Dmod::DisconnectInputApis(Dmod_Context_t* Context)
{
    return Dmod_DisconnectInputApis(Context);
}

bool Dmod::DisconnectAllApis(Dmod_Context_t* Context)
{
    return Dmod_DisconnectAllApis(Context);
}

void* Dmod::GetFunction(Dmod_Context_t* Context, const char* Signature)
{
    return Dmod_GetFunction(Context, Signature);
}

void Dmod::Preinit(Dmod_Context_t* Context)
{
    Dmod_Preinit(Context);
}

int Dmod::Init(Dmod_Context_t* Context, const Dmod_Config_t* Config)
{
    return Dmod_Init(Context, Config);
}

int Dmod::Main(Dmod_Context_t* Context, int argc, char *argv[])
{
    return Dmod_Main(Context, argc, argv);
}

int Dmod::Deinit(Dmod_Context_t* Context)
{
    return Dmod_Deinit(Context);
}

int Dmod::Signal(Dmod_Context_t* Context, int SignalNumber)
{
    return Dmod_Signal(Context, SignalNumber);
}

int Dmod::Irq(Dmod_Context_t* Context, int IrqNumber)
{
    return Dmod_Irq(Context, IrqNumber);
}

void Dmod::IrqAll(int IrqNumber)
{
    Dmod_IrqAll(IrqNumber);
}

uint64_t Dmod::GetStackSize(Dmod_Context_t* Context)
{
    return Dmod_GetStackSize(Context);
}

Dmod_ModuleType_t Dmod::GetModuleType(Dmod_Context_t* Context)
{
    return Dmod_GetModuleType(Context);
}

bool Dmod::Enable(Dmod_Context_t* Context, bool Force, const Dmod_Config_t* Config)
{
    return Dmod_Enable(Context, Force, Config);
}

bool Dmod::Disable(Dmod_Context_t* Context, bool Force)
{
    return Dmod_Disable(Context, Force);
}

bool Dmod::IsEnabled(Dmod_Context_t* Context)
{
    return Dmod_IsEnabled(Context);
}

int Dmod::Run(Dmod_Context_t* Context, int argc, char *argv[])
{
    return Dmod_Run(Context, argc, argv);
}