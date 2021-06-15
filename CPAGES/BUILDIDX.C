/*
 *
 * buildidx.c
 * Tuesday, 4/22/1997.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <io.h>
#include <GnuType.h>
#include <GnuMem.h>
#include <GnuArg.h>
#include <GnuDir.h>
#include <GnuMisc.h>
#include <GnuStr.h>
#include <GnuCfg.h>
#include <GnuFile.h>
#include "files.h"
#include "buffers.h"

BOOL bDUMP;      

void WriteBody  (FILE *fp, PFIL pFil)
   {
   if (!pFil)
      return;
   WriteBody  (fp, pFil->left);
   WriteBlock (fp, pszBODY, pFil);
   WriteBody  (fp, pFil->right);
   }


void WriteIndex (PFIL pFil, PSZ pszOutFile)
   {
   FILE *fp;

   if (!(fp = fopen (pszOutFile, "wt")))
      Error ("Cannot open output file: %s", pszOutFile);

   WriteBlock (fp, pszPRE, NULL);
   WriteBody  (fp, pFil);
   WriteBlock (fp, pszPOST, NULL);
   fclose (fp);
   }


void DumpFiles (PFIL pFil)
   {
   CHAR szTmp [256];

   if (!pFil)
      return;

   DumpFiles (pFil->left);
   printf ("%-14s %-14s %s %6lu %s\n", pFil->pszOld, pFil->pszNew, 
            StrFDateString (szTmp, pFil->fDate), pFil->ulSize, pFil->pszDesc);
   DumpFiles (pFil->right);
   }

void Usage (void)
   {
   printf ("BuildIDX  C Source index WEB page creation utility              %s\n\n", __DATE__);
   printf ("USAGE:  BuildIdx [options] files files ...\n\n");
   printf ("WHERE:  [options] are 0 or more of:\n");
   printf ("           /? ................ This help.\n");
   printf ("           /Cfg=file ......... Specify CFG file (def: BUILDIDX.CFG).\n");
   printf ("           /Var=string ....... supply value for @# param.\n");
   printf ("           /Dump ............. Dump all loaded files only.\n");
   printf ("           /Sort=method ...... Sort file index (Ext,Name,Size,Date,x) .\n");
   printf ("           /Out=file ......... Specify Output file name (def: index.htm)\n");
   printf ("\n");
   printf ("The [HEADER],[BODY], and [FOOTER] sections of the cfg file can \n");
   printf ("have the following replacable parameters:\n");
   printf ("      @oldfile ...... The input file name\n");
   printf ("      @newfile ...... The html file name\n");
   printf ("      @fileDate ..... The input file date\n");
   printf ("      @filetime ..... The input file time\n");
   printf ("      @filedesc ..... The input file 4dos description\n");
   printf ("      @date ......... todays date\n");
   printf ("      @time ......... current time\n");
   printf ("      @0 thru @9 .... The nth /Var parameter specified on cmd line.\n");
   printf ("\n");
   printf ("EXAMPLE: BUILDIDX *.c *.h *.exe\n");
   exit (0);
   }


/*
 * make pre post and body from a ini file
 */
int _cdecl main (int argc, char *argv[])
   {
   PFIL pFil = NULL;
   UINT i;
   PSZ  pszOutFile;

   if (ArgBuildBlk ("*^Cfg% *^Var% *^Dump *^Sort% *^Out% ?"))
      Error ("%s", ArgGetErr ());

   if (ArgFillBlk (argv))
      Error ("%s", ArgGetErr ());

   if (ArgFillBlk2 (getenv ("BUILDIDX")))
      return Error ("env BUILDIDX ERROR: %s", ArgGetErr ());

   ArgDump ();

   if (ArgIs ("?") || !ArgIs (NULL))
      Usage ();

   bDUMP = ArgIs ("Dump");
   cSORT = (ArgIs ("Sort") ? *ArgGet ("Sort", 0) : SORT_NAME);

   pszOutFile = (ArgIs ("Out") ? ArgGet ("Out", 0) : "index.htm");

   LoadCfg ("buildidx.cfg", ArgGet ("Cfg", 0), argv[0]);

   for (i=0; i<ArgIs (NULL); i++)
      pFil = AddFiles (ArgGet (NULL, i), pFil);

   MakeNames (pFil, pFil, "");

   if (bDUMP)
      DumpFiles (pFil);
   else
      WriteIndex (pFil, pszOutFile);

   printf ("Done.");
   return 0;
   }

