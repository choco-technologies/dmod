SECTIONS
{
    INCLUDE dmod-common.ld
}

/* Keep the platform's default executable layout and add only the DMOD API
 * registration sections.  Replacing the complete system linker script makes
 * PIE executables invalid on platforms such as AArch64. */
INSERT AFTER .data;
