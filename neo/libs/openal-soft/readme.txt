OpenAL Soft Binary Distribution

These binaries are provided as a convenience. Users and developers may use it
so they can use OpenAL Soft without having to build it from source.

Note that it is still expected to install the OpenAL redistributable provided
by Creative Labs (at http://openal.org/), as that will provide the "router"
OpenAL32.dll that applications talk to, and may provide extra drivers for the
user's hardware. The DLLs provided here will simply add additional devices for
applications to select from. If you do not wish to use the redistributable,
then rename soft_oal.dll to OpenAL32.dll (note: even the 64-bit DLL should be
named OpenAL32.dll)... just be aware this will prevent other system-installed
OpenAL implementations from working.

To use the 32-bit DLL, copy it from the Win32 folder to the folder OpenAL32.dll
is installed in.
For 32-bit Windows, the Win32 DLL will typically go into the system32 folder.
For 64-bit Windows, the Win32 DLL will typically go into the SysWOW64 folder.

To use the 64-bit DLL, copy it from the Win64 folder to the folder OpenAL32.dll
is installed in. This will typically be the system32 folder.

The included openal-info32.exe and openal-info64.exe programs can be used to
tell if the OpenAL Soft DLL is being detected. It should be run from a command
shell, as the program will exit as soon as it's done printing information.

Have fun!
