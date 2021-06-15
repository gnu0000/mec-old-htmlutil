These are the files i used to create the source code web pages

        BUILDLST.EXE
        CPAGES.EXE
        post.htm
        pre.htm


CPAGES.EXE is a program that creates html pages from C and H source
files.  
BUILDLST.EXE is a program that builds link files that are needed by
the CPAGES.EXE program.


Building linked source code pages it is a 2 step process..



1> BUILD LINK FILES
   Link files are CSV files of the form:
        
        identifier, filename, line#

   one entry per line.  Identifier are the words that are made into
   links. the file name/line is the link destination.  

   CPAGES expects to have 2 link files, a file containing links to
   function definitions (from C files) and a link file containing
   function declarations (from H files). These files can be built
   by typing:

        BUILDLST /H    
        -and-
        BUILDLST /C


2> CREATE PAGES

   CPAGES actually creates the pages.  
   Fn references in the header point to the fn declarations in the header.  
   Fn declarations in the header point to the fn definitions in the C file.
   Fn references in the C file point to the Fn definitions in the C file.
   Fn definitions in the C file point back to Fn declarations in the header.
   got it?
   
   This program expects several things as input.
        > A Definition link file, defaults to C.LNK (built by BUILDLNK)
        > A Declaration link file, defaults to H.LNK (built by BUILDLNK)
        > A prefix file containing HTML, defaults to PRE.HTM (provided)
        > A suffix file containing HTML, defaults to POST.HTM (provided)

   The link files were described above.  The pre and post files contain
   stuff you want on each page, like the page title, the background, 
   and the links back to the index or the home page.  Look at the example
   files provided here.  Note that there are several allowable replacements
   you can use in these files, for example you can use @infile which is
   replaced by the name of the C or H file used to generate the page.  
   Type CPAGES /? to see the list of replacable parameters.

   To create the pages:

        > Edit post.htm and change the BACK link name to be something
        other than "EBS Source Index"

        > type CPAGES 


There are several options for each program.  You can type
BUILDLST /?  or  CPAGES /? for help on options.

For example to make the pages for GNULIB and EBS I typed the following:

        CD \HTML\ITIUTIL\GNULIB
        BUILDLST /C
        BUILDLST /H
        CPAGES
        CD \HTML\EBSSRC
        BUILDLST /C
        BUILDLST /H
        CPAGES /CLINKS=C.LNK /CLINKS=..\ITIUTIL\GNULIB\H.LNK

Note that this last cpages command used the /CLINK parameter. this is
so that the C source for EBS would contain links to the GNULIB library 
headers.


============================================================================

        MAKEPAGE.EXE


This is a general purpose program to create linked web pages from text files.
It is a generalized version of the CPAGES program above.

   This program expects several things as input.
        > A prefix file containing HTML, defaults to PRE.HTM
        > A suffix file containing HTML, defaults to POST.HTM
        > A link file, although optional, this is the whole point









