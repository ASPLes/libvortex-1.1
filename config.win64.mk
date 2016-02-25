# especific configurations
# export version_prefix    = -MinGW32
export version_prefix    = -MinGW64

# platfor bits 
# export platform_bits     = 32
export platform_bits     = 64

# some options used at 32
# export add_stdcall_alias = "--add-stdcall-alias "
export add_stdcall_alias = 

# some options used at 32
# export export_all_symbols = "--export-all-symbols"
export export_all_symbols = 

# library used by minwgw compiler
export libgcc_s_dw2       = c:/Users/acinom/home/mingw-installer/bin/libgcc_s_dw2-1.dll

# Flags to compile OPENSSL
# Uncomment the following lines, placing the libraries following the path
# schema provided to get support OpenSSL
# 32bits
# export OPENSSL_FLAGS = -DENABLE_TLS_SUPPORT -Ic:/openssl-098k/include
# export OPENSSL_LIBS = c:/openssl-098k/bin/libeay32.dll c:/openssl-098k/bin/ssleay32.dll
export OPENSSL_FLAGS = -DENABLE_TLS_SUPPORT -Ic:/openssl/include
export OPENSSL_LIBS = c:/openssl/bin/libeay32.dll c:/openssl/bin/ssleay32.dll

# Uncomment the following lines, placing the libraries following the path
# schema provided to get support for GSASL
export GSASL_FLAGS = -DENABLE_SASL_SUPPORT -Ic:/gsasl/gsasl-1.8.0/include
export GSASL_LIBS = c:/gsasl/gsasl-1.8.0/bin/libgsasl-7.dll \
	            c:/gsasl/gsasl-1.8.0/bin/libgcrypt-11.dll \
                    c:/gsasl/gsasl-1.8.0/bin/libgpg-error-0.dll 

# Readline support
export readline_libs = c:/readline/readline-6.2/bin/libreadline6.dll
export readline_flags = -Ic:/readline/readline-6.2/include 
# export readline_libs = c:/af-arch-depends/bin/readline5.dll

# Readline support
export nopoll_libs = c:/Users/acinom/home/mingw-installer/home/acinom/nopoll/src/libnopoll.dll
export nopoll_flags = -Ic:/Users/acinom/home/mingw-installer/home/acinom/nopoll/src/


# optional nsis installation
# makensis              = c:/ARCHIV~1/NSIS/makensis.exe
makensis		= "C:/Program Files (x86)/NSIS/makensis.exe"

# cc compiler to use usually gcc.exe
export CC          = x86_64-w64-mingw32-gcc.exe
