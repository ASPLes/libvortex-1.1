# especific configurations
export version_prefix    = -MinGW32
# export version_prefix    = -MinGW64

# platfor bits 
export platform_bits     = 32
# export platform_bits     = 64

# link indication for .lib files
export link_machine      = X86
# export link_machine      = X64
# /MACHINE:{ARM|EBC|IA64|MIPS|MIPS16|MIPSFPU|MIPSFPU16|          SH4|THUMB|X64|X86}
# https://msdn.microsoft.com/en-us/library/5wy54dk2(v=vs.90).aspx


# some options used at 32
export add_stdcall_alias = --add-stdcall-alias 
# export add_stdcall_alias = 

# some options used at 32
export export_all_symbols = --export-all-symbols
# export export_all_symbols = 

# location of the libgcc_s_dw2-1.dll
export libgcc_s_dw2       = c:/mingw/bin/libgcc_s_dw2-1.dll

# Flags to compile OPENSSL
# Uncomment the following lines, placing the libraries following the path
# schema provided to get support OpenSSL
# 32bits
# export OPENSSL_FLAGS = -DENABLE_TLS_SUPPORT -Ic:/openssl-098k/include
# export OPENSSL_LIBS = c:/openssl-098k/bin/libeay32.dll c:/openssl-098k/bin/ssleay32.dll
export OPENSSL_FLAGS = -DENABLE_TLS_SUPPORT -Ic:/openssl/include
export OPENSSL_LIBS = c:/openssl-098h/bin/libeay32.dll c:/openssl-098h/bin/ssleay32.dll

# Uncomment the following lines, placing the libraries following the path
# schema provided to get support for GSASL
export GSASL_FLAGS = -DENABLE_SASL_SUPPORT -Ic:/gsasl/gsasl-1.0/include
export GSASL_LIBS = c:/gsasl/gsasl-1.0/bin/libgsasl-7.dll \
	            c:/gsasl/gsasl-1.0/bin/libgcrypt-11.dll \
                    c:/gsasl/gsasl-1.0/bin/libgpg-error-0.dll 

export readline_flags = -Ic:/af-arch-depends/include
export readline_libs = c:/af-arch-depends/bin/readline5.dll

# nopoll support
export nopoll_libs = c:/Users/acinom/home/mingw-installer/home/acinom/nopoll/src/libnopoll.dll
export nopoll_flags = -Ic:/Users/acinom/home/mingw-installer/home/acinom/nopoll/src/

# nopoll support
export AXL_LIBS = /home/acinom/libaxl/src/libaxl.dll
export AXL_CFLAGS = -I/home/acinom/libaxl/src/



# optional nsis installation
makensis              = c:/ARCHIV~1/NSIS/makensis.exe
# makensis		= "C:/Program Files (x86)/NSIS/makensis.exe"

# cc compiler to use usually gcc.exe
export CC          = gcc.exe
