cmake_minimum_required(VERSION 3.18)

dmod_add_tool(dif_test main.c)

# Add test_direct_access tool
dmod_add_tool(test_direct_access test_direct_access.c)
