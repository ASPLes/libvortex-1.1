
              -----------------------------------------
                     Vortex Library for Windows
               Advanced Software Production Line, S.L.
              -----------------------------------------

1. Introducion

   This package includes all pieces necessary to start developing BEEP
   based programs and to test how Vortex Library works.

   This package have been tested to work on Microsoft Windows XP
   platform, but it shouldn't be any problem on the following windows
   platforms including 2000, 2003 and advance server editions.

   If you feel you have found something missing or something that
   could be improved, comments and suggestions, drop us an email:

      vortex@lists.aspl.es

   Or use the Vortex Library development mailing list:

      http://lists.aspl.es/cgi-bin/mailman/listinfo/vortex

   You'll find more information at:

      http://vortex.aspl.es

   Enjoy the Vortex!

2. Looking around to test the library

   If you are interested on testing the library, ensure you select
   test applications to be installed at the installation process. 

   Then check the "bin" directory found inside your selected
   installation directory.

   Inside that directory you'll find, among other things, the
   following programs:

       - vortex-client.exe :: 

         This is the vortex client tool, the one used to debug BEEP
         peers, testing TLS support and SASL. It has support for a
         basic frame generator, managing channels, query current
         channel/connection status etc. You'll have to use this
         program as a BEEP initiator for the following tests.

       - vortex-listener.exe, vortex-simple-listener.exe,
         vortex-sasl-listener.exe, vortex-tls-listener.exe,
         vortex-omr-server.exe :: 

         All of them are server side implementations (that is, BEEP
         listeners) that implements some especific function. In the
         case the TLS implemention is required to be tested, you can
         try with the vortex-tls-listener.exe.

         Just launch the server. You'll see the port where the server
         is starting to listen. Then use the vortex-client tool to
         connect and play with it.
      
       - vortex-regression-client.exe, vortex-regression-listener.exe ::
         
         These are the tools used to check the library function on
         each release. It is a regression test that checks several
         functions from the library and must execute properly on your
         platform.

         Please report any wrong function found while using these
         regression test. If you can, attach a patch fixing bug you
         have found.

	  In general, all development to check and improve the library
	  is done through these files (vortex-regression-client.c and
	  vortex-regression-listener.c).

3. Using the package as a support installation for your programs

   If you are trying to deliver BEEP based programs using Vortex
   Library, you can bundle your installation with this package. The
   "bin" directory is the only one piece of insterest: it contains all
   dll files that your binaries will require at runtime installations.

4. Using the package to develop

   Inside the installation provided, the following directories
   contains required files to develop:

    - bin: dll files and data files. Your program being developed will
      require them. 

      You can modify your environment variable PATH to include the bin
      directory of your Vortex Library installation so your dinamic
      library loader can find them.

    - lib: import libraries for gcc and VS installations. They are
      provided either by the usual windows way (.lib) or the gcc way
      (.dll.a) through the mingw environment.

    - include: this directory contains vortex headers (and all
      required headers that the vortex depends on).

5. But, how do I ....

   Use the mailing list to reach us if you find that some question is
   not properly answered on this readme file. 

   You have contact information at the top of this file. Feel free to
   drop us an email!

   Vortex is commercially supported. In the case you want professional
   support see http://www.aspl.es/vortex/professional.html

6. That's all

   We hope you find useful this library, try to keep us updated if you
   develop new products, profiles, etc. We will appreciate that,
   making your produt/project to appear at the Vortex main site.

   Let it BEEP!

-- 
Advanced Software Production Line, S.L.
C/ Antonio Suarez Nº10, Edificio Alius A, Despacho 102
Alcalá de Henares (28802) Madrid, Spain

Francis Brosnan Blázquez
<francis@aspl.es>
2008/07/01
