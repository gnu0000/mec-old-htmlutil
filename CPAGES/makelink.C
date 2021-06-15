/*
 *
 * buildlst.c
 * Thursday, 8/29/1996.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <GnuType.h>
#include <GnuFile.h>
#include <GnuStr.h>
#include <GnuMisc.h>
#include <GnuArg.h>
#include <GnuDir.h>
#include "Files.h"

PSZ pszTypes [] =
   {"void",   "int",
    "uint",   "psz",
    "char",   "uchar",
    "bool",   "short",
    "ushort", "=",
    NULL};


void StrLower (PSZ psz)
   {
   for (; psz && *psz; psz++)
      *psz = tolower (*psz);
   }


void WriteLine (FILE *fp, PSZ pszID, PSZ pszFile, ULONG ulLine)
   {
   if (!*pszID)
      pszID = "XXXXXXXX";
   fprintf (fp, "%-24s,%12s,%4lu\n", pszID, pszFile, ulLine);
   }


BOOL GetFunction (FILE *fpIn, FILE *fpOut, PFIL pFil, PSZ pszLine)
   {
   ULONG ulLine;
   PSZ   psz;
   UINT  i, j, uLen;
   BOOL  bType;

   if (!__iscsymf (*pszLine))
      return FALSE;
   if (!(psz = strchr (pszLine, '(')))
      return FALSE;
   *psz = '\0';
   StrClip (pszLine, " \t");
   if (!(uLen = strlen (pszLine)))
      return FALSE;

   for (i= uLen; i; i--)
      {
      if (!strchr (" \t(){}*&", pszLine[i-1]))
         continue;

//      if (pszLine[i-1] != ' ' && pszLine[i-1] != '\t')
//         continue;

      bType = FALSE;
      for (j=0; pszTypes[j]; j++)
         if (!stricmp (pszTypes[j], pszLine+i))
            bType = TRUE;
      if (bType)
         break;

      ulLine = FilGetLine ();
      WriteLine (fpOut, pszLine+i, pFil->pszNew, ulLine);
      break;
      }
   return TRUE;
   }


BOOL GetTypedef (FILE *fpIn, FILE *fpOut, PFIL pFil, PSZ pszLine)
   {
   PSZ   psz, p1;
   INT   i, iBrace;
   ULONG ulLine;
   CHAR  szIdent [128];

   psz = StrSkipBy (pszLine, " \t");
   if (strncmp (psz, "typedef", 7))
      return FALSE;
   ulLine = FilGetLine ();

   iBrace = 0;
   while (TRUE)
      {
      psz = StrClip (psz, " \t\n");
      for (p1 = psz; *p1; p1++)
         iBrace += (*p1 == '{') - (*p1 == '}');

      if (psz[strlen(psz)-1] == ';' && !iBrace)
         break;

      if (FilReadLine (fpIn, pszLine, "", 512) == (UINT)-1)
         Error ("Could not handle typedef line: %lu", FilGetLine());
      psz = pszLine;
      }
   for (i = strlen (psz)-1; i && !strchr (" \t*}{&", psz[i]); i--)
      ;
   psz+= i+1;
   StrGetIdent (&psz, szIdent, 1, FALSE);
   WriteLine (fpOut, szIdent, pFil->pszNew, ulLine);
   return TRUE;
   }
   

BOOL GetDefine (FILE *fpIn, FILE *fpOut, PFIL pFil, PSZ pszLine)
   {
   PSZ psz;
   CHAR szIdent [128];

   psz = StrSkipBy (pszLine, " \t");
   if (*psz != '#')
      return FALSE;
   if (strnicmp (psz, "#define", 7))
      return FALSE;

   psz += 8;
   StrGetIdent (&psz, szIdent, 1, FALSE);
   WriteLine (fpOut, szIdent, pFil->pszNew, FilGetLine ());
   return TRUE;
   }


void ProcessFile (FILE *fpIn, FILE *fpOut, PFIL pFil)
   {
   CHAR  szLine[512];

   FilSetLine (0);
   while (FilReadLine (fpIn, szLine, "", sizeof (szLine)) != (UINT)-1)
      {
      if (GetFunction (fpIn, fpOut, pFil, szLine))
         continue;

      if (GetTypedef (fpIn, fpOut, pFil, szLine))
         continue;

      if (GetDefine (fpIn, fpOut, pFil, szLine))
         continue;
      }
   }


UINT ProcessFiles (PFIL pFil, FILE *fpOut)
   {
   FILE *fpIn;
   UINT uFiles;

   if (!pFil)
      return 0;

   uFiles = ProcessFiles (pFil->left, fpOut);
   
   if (!(fpIn = fopen (pFil->pszOld, "rt")))
      Error ("Unable to open input file %s", pFil->pszOld);
   printf ("processing file %s\n", pFil->pszOld);

   ProcessFile (fpIn, fpOut, pFil);
   uFiles++;
   fclose (fpIn);

   uFiles += ProcessFiles (pFil->right, fpOut);
   return uFiles;
   }


void AddReferences (PFIL pFil, FILE *fp)
   {
   if (!pFil)
      return;
       
   AddReferences (pFil->left, fp);
   Lower (pFil->pszOld);
   Lower (pFil->pszNew);

   WriteLine (fp, pFil->pszOld, pFil->pszNew, 1);
   AddReferences (pFil->right, fp);
   }


void Usage (void)
   {
   printf ("MakeLink  Builds lists of C Definitions and declaration positions %s\n\n", __DATE__);
   printf ("USAGE:  MakeLink [options] files..\n\n");
   printf ("WHERE:  files .... are the C or H files to process.\n");
   printf ("        [options] are 0 or more of:\n");
   printf ("           /Links=file ....... Specify auto link file to build.\n");
   printf ("           /FileRef=files .... Include references to other files\n");
   printf ("\n");
   printf ("Basic Usage:\n");
   printf ("\n");
   printf ("  BUILDLST /Links=C.LNK /FileRef=*.h *.c\n");
   printf ("  This builds the C.LNK file which contains links to fn definitions\n");
   printf ("  and links to header files\n");
   printf ("\n");
   printf ("  BUILDLST /Links=H.LNK *.h\n");
   printf ("  This builds the H.LNK file which contains links to fn declarations\n");
   printf ("\n");
   printf ("See CPAGES for how to use these files.\n");
   printf ("\n");
   printf ("Shortcuts:\n");
   printf ("  MakeLink /C   -  Use default options and create c.lnk\n");
   printf ("  MakeLink /H   -  Use default options and create h.lnk\n");
   exit (0);
   }


int _cdecl main(int argc, char *argv[])
   {
   CHAR   szLinksFile[256];
   CHAR   szDir      [256];
   FILE   *fpOut;
   UINT   i, uFiles;
   PFIL   pFil = NULL, pFilR = NULL;

   if (ArgBuildBlk ("*^Links% *^FileRef=% *^Sort% ? *^C *^H"))
      Error ("%s", ArgGetErr ());

   if (ArgFillBlk (argv))
      Error ("%s", ArgGetErr ());

   /*--- shortcut options ---*/
   if (ArgIs ("C"))
      ArgFillBlk2 ("/Links=C.LNK /FileRef=*.h *.c");
   else if (ArgIs ("H"))
      ArgFillBlk2 ("/Links=H.LNK *.h");

   if (ArgIs ("?") || !ArgIs (NULL))
      Usage ();

   strcpy (szLinksFile, ArgIs ("Links") ? ArgGet ("Links", 0) : "links.lnk");

   cSORT = (ArgIs ("Sort") ? *ArgGet ("Sort", 0) : SORT_NAME);

   if (!(fpOut= fopen (szLinksFile, "wt")))
      Error ("Unable to open links file %s", szLinksFile);

   for (i=0; i< ArgIs (NULL); i++)
      {
      DirSplitPath (szDir,  ArgGet (NULL, i), DIR_DRIVE | DIR_DIR);
      pFil = AddFiles (ArgGet (NULL, i), pFil);
      MakeNames (pFil, pFil, szDir);
      }
   uFiles = ProcessFiles (pFil, fpOut);

   for (i=0; i < ArgIs ("FileRef"); i++)
      {
      DirSplitPath (szDir,  ArgGet ("FileRef", i), DIR_DRIVE | DIR_DIR);
      pFilR = AddFiles (ArgGet ("FileRef", i), pFilR);
      MakeNames (pFilR, pFilR, szDir);
      }
   AddReferences (pFilR, fpOut);

   fclose (fpOut);
   printf ("%d files processed", uFiles);
   return 0;
   }


