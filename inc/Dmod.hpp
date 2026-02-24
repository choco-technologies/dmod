#ifndef DMOD_HPP
#define DMOD_HPP

extern "C"
{
    #include <dmod.h>
}

class Dmod 
{
public:
    Dmod() = default;
    virtual ~Dmod() = default;
    
    /**
     * @brief Loads a module from a file.
     * 
     * @param Path Path to the module file.
     * 
     * @return Dmod_Context_t* Pointer to the module context.
     */
    Dmod_Context_t* LoadFile(const char* Path);
    Dmod_Context_t* Load(const void* Data, size_t Size);
    bool Unload(Dmod_Context_t* Context, bool Force);
    bool Initialize(size_t NumIrqs, size_t MaxHandlersPerIrq);
    bool ConnectApi(Dmod_Api_t* OutputsApi, Dmod_Api_t* InputsApi);
    bool DisconnectApi(Dmod_Api_t* OutputsApi, Dmod_Api_t* InputsApi);
    bool ConnectOutputApis(Dmod_Context_t* Context);
    bool ConnectInputApis(Dmod_Context_t* Context);
    bool ConnectAllApis(Dmod_Context_t* Context);
    bool DisconnectOutputApis(Dmod_Context_t* Context);
    bool DisconnectInputApis(Dmod_Context_t* Context);
    bool DisconnectAllApis(Dmod_Context_t* Context);
    void* GetFunction(Dmod_Context_t* Context, const char* Signature);
    void Preinit(Dmod_Context_t* Context);
    int Init(Dmod_Context_t* Context, const Dmod_Config_t* Config);
    int Main(Dmod_Context_t* Context, int argc, char *argv[]);
    int Deinit(Dmod_Context_t* Context);
    int Signal(Dmod_Context_t* Context, int SignalNumber);
    int Irq(Dmod_Context_t* Context, int IrqNumber);
    void IrqAll(int IrqNumber);
    uint64_t GetStackSize(Dmod_Context_t* Context);
    Dmod_ModuleType_t GetModuleType(Dmod_Context_t* Context);
    bool Enable(Dmod_Context_t* Context, bool Force, const Dmod_Config_t* Config);
    bool Disable(Dmod_Context_t* Context, bool Force);
    bool IsEnabled(Dmod_Context_t* Context);
    int Run(Dmod_Context_t* Context, int argc, char *argv[]);
private:

};

#endif // DMOD_HPP