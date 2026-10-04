#if defined(DMOD_MODULE) || defined(DMOD_SYSTEM)
#error "The SDK leaked module definitions into an unrelated target"
#endif
int ordinary(void)
{
    return 42;
}
