/*
 *
 * cpages.c
 * Tuesday, 8/27/1996.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <io.h>
#include <GnuType.h>
#include <GnuFile.h>
#include <GnuStr.h>
#include <GnuArg.h>
#include <GnuMem.h>
#include <GnuMisc.h>
#include <GnuDir.h>
#include "files.h"
#include "buffers.h"

typedef struct _lnk
   {
   PSZ  pszSrcName;  // string to make a HREF
   UINT uSrcLen   ;  // len of pszSrcName
   PSZ  pszFile   ;  // file to point to
   UINT uLine     ;  // line to point to
   PSZ  pszGenKey ;  // link key
   PSZ  pszHREF   ;  // HREF string in pre-build form
   UINT uHREFLen  ;  // len of pszHREF

   struct _lnk *leftT;
   struct _lnk *rightT;

   struct _lnk *leftN;
   struct _lnk *rightN;
   } LNK;
typedef LNK *PLNK;


typedef struct _lnks
   {
   PLNK plnkT; // tag tree   T=ordered by tag str
   PLNK plnkN; // name tree  N=ordered by line/file
   } LNKS;
typedef LNKS *PLNKS;

BOOL bCASE;
CHAR szBUFF  [0x4000];
CHAR szBUFF2 [0x4000];

PSZ  pszPRE;
PSZ  pszPOST;

LNKS lnksC;
LNKS lnksH;
UINT uLastCount = 0;

void Usage (void)
   {
   printf ("CPages  C Source WEB page creation utility              %s\n\n", __DATE__);
   printf ("USAGE:  CPAGES [options]\n\n");
   printf ("WHERE:  [options] are 0 or more of:\n");
   printf ("           /? ................ This help.\n");
   printf ("           /CLinks=file ...... Specify auto link file (def: C.LNK).\n");
   printf ("           /HLinks=file ...... Specify auto link file.(def: H.LNK).\n");
   printf ("           /Dump ............. Dump all loaded links only.\n");
   printf ("           /Cfg=file ......... Specify CFG file (def: CPAGES.CFG).\n");
   printf ("           /Var=string ....... supply value for @# param.\n");
   printf ("           /NoCase ........... all string compares are not case-sensitive.\n");
   printf ("\n");
   printf ("the prefile and post file can have the following replacable parameters:\n");
   printf ("      @infile ....... The input file name\n");
   printf ("      @outfile ...... The output file name\n");
   printf ("      @infilebase ... The input file name without the extension\n");
   printf ("      @outfilebase .. The output file name without the extension\n");
   printf ("      @fileDate ..... The input file date\n");
   printf ("      @filetime ..... The input file time\n");
   printf ("      @filedesc ..... The input file 4dos description\n");
   printf ("      @date ......... todays date\n");
   printf ("      @time ......... current time\n");
   printf ("      @0 thru @9 .... The nth /Var parameter specified on cmd line.\n");
   printf ("\n");
   printf ("Autolink (CSV) file format is:\n");
   printf ("stringtolink, DestFile, DestLine\n");
   printf ("\n");
   exit (0);
   }

INT Mystrcmp (PSZ psz1, PSZ psz2)
   {
   if (bCASE)
      return strcmp (psz1, psz2);
   stricmp (psz1, psz2);
   }

INT Mystrncmp (PSZ psz1, PSZ psz2, UINT uLen)
   {
   if (bCASE)
      return strncmp (psz1, psz2, uLen);
   strnicmp (psz1, psz2, uLen);
   }

/***************************************************************************/
/*                                                                         */
/*                                                                         */
/*                                                                         */
/***************************************************************************/


UINT AddToTTree (PLNK plnkTop, PLNK plnkNew)
   {
   PLNK *pplnk;
   INT  i;

   if (!(i = Mystrcmp (plnkNew->pszSrcName, plnkTop->pszSrcName)))
      return printf ("Duplicate link tag: %s\n", plnkNew->pszSrcName);
   pplnk = (i < 0 ? &(plnkTop->leftT) : &(plnkTop->rightT));
   if (*pplnk)
      return AddToTTree (*pplnk, plnkNew);
   *pplnk = plnkNew;
   return 0;
   }


UINT AddToNTree (PLNK plnkTop, PLNK plnkNew)
   {
   PLNK *pplnk;
   INT  i;

   if (!(i = (INT)plnkNew->uLine - (INT)plnkTop->uLine))
      i = Mystrcmp (plnkNew->pszFile, plnkTop->pszFile);
   pplnk = (i < 0 ? &(plnkTop->leftN) : &(plnkTop->rightN));
   if (*pplnk)
      return AddToNTree (*pplnk, plnkNew);
   *pplnk = plnkNew;
   return 0;
   }


PLNK FindTNode (PLNK plnkTop, PSZ pszSrc)
   {
   INT  i;
   UINT c;

   if (!plnkTop)
      return NULL;
   if (!(i = Mystrncmp (pszSrc, plnkTop->pszSrcName, plnkTop->uSrcLen)))
      {
      c = pszSrc[plnkTop->uSrcLen];
      if (isalnum (c) || c == '_')
         i = 1;
      else
         return plnkTop;
      }
   if (i < 0)
      return FindTNode (plnkTop->leftT, pszSrc);
   return FindTNode (plnkTop->rightT, pszSrc);
   }


PLNK FindNNode (PLNK plnkTop, PSZ pszFile, UINT uLine)
   {
   INT  i;

   if (!plnkTop)
      return NULL;
   if (!(i = (INT)uLine - (INT)plnkTop->uLine))
      if (!(i = Mystrcmp (pszFile, plnkTop->pszFile)))
         return plnkTop;
   if (i < 0)
      return FindNNode (plnkTop->leftN, pszFile, uLine);
   return FindNNode (plnkTop->rightN, pszFile, uLine);
   }


/***************************************************************************/
/*                                                                         */
/*                                                                         */
/*                                                                         */
/***************************************************************************/


void StoreLink (PPSZ ppsz, PSZ pszFileDir, 
                UINT uCols, PLNKS plnks, PSZ pszType)
   {
   PLNK plnk;
   CHAR szBuff [256];

   if (!(plnk = calloc (1, sizeof (LNK))))
      Error ("cant malloc");

   Lower (ppsz[1]);
   Lower (pszFileDir);

   plnk->pszSrcName = ppsz[0];
   plnk->uSrcLen    = strlen(ppsz[0]);

   sprintf (szBuff, "%s%s", pszFileDir, ppsz[1]);
   plnk->pszFile    = strdup (szBuff);
   plnk->uLine      = atoi(ppsz[2]);

   plnk->pszGenKey  = plnk->pszSrcName; // at least for now

   sprintf (szBuff, "<A HREF=\"%s#%s\">%s</A>", 
                     plnk->pszFile, plnk->pszGenKey, plnk->pszSrcName);

   plnk->pszHREF    = strdup (szBuff);
   plnk->uHREFLen   = strlen(plnk->pszHREF);

   MemFreeData (ppsz[1]); 
   MemFreeData (ppsz[2]); 
   MemFreeData (ppsz); 


   /*--- add to T tree ---*/
   if (!plnks->plnkT)
      plnks->plnkT = plnk;
   else
      AddToTTree (plnks->plnkT, plnk);

   /*--- add to N tree ---*/
   if (!plnks->plnkN)
      plnks->plnkN = plnk;
   else
      AddToNTree (plnks->plnkN, plnk);
   }



void ReadAutolinkFile (PLNKS plnks, PSZ pszFile, PSZ pszType)
   {
   FILE *fp;
   PPSZ ppsz;
   PSZ  psz;
   UINT uCols;
   CHAR szFileDir[256];

   DirSplitPath (szFileDir, pszFile, DIR_DRIVE | DIR_DIR);
   while (psz = strchr (szFileDir, '\\'))
      *psz = '/';
   Lower (szFileDir);

   if (!(fp = fopen (pszFile, "rt")))
      Error ("Cannot open autolink file %s", pszFile);
   printf ("Reading links file: %s\n", pszFile);

   while (FilReadLine (fp, szBUFF, "", sizeof (szBUFF)) != (UINT)-1)
      {
      ppsz = StrMakePPSZ  (szBUFF, ",\n", TRUE, TRUE, &uCols);
      if (uCols < 3 || !ppsz[0] || !ppsz[1] || !ppsz[2])
         {
         MemFreePPSZ (ppsz, uCols);
         continue;
         }
      StoreLink (ppsz, szFileDir, uCols, plnks, pszType);
      }
   }


void DumpLinks (PLNK plnk)
   {
   if (!plnk)
      return;

   DumpLinks (plnk->leftT);
   printf ("%s:%3.3d %s\n", plnk->pszFile, plnk->uLine, plnk->pszHREF);
   DumpLinks (plnk->rightT);
   }


/***************************************************************************/
/*                                                                         */
/*                                                                         */
/*                                                                         */
/***************************************************************************/


PLNK MaybeWriteNAME (FILE *fpOut, PLNKS plnks, PSZ pszInFile, PSZ pszOutFile)
   {
   PLNK plnk;

   if (plnk = FindNNode (plnks->plnkN, pszOutFile, (UINT)FilGetLine ()))
      fprintf (fpOut, "<A NAME=\"%s\"></A>", plnk->pszGenKey);
   return plnk;
   }


int fputHTMLc (UINT c, FILE *fp)
   {
   if (c == '<')
      return fputs ("&lt", fp);
   if (c == '>')
      return fputs ("&gt", fp);
   if (c == '&')
      return fputs ("&amp", fp);
   if (c == '"')
      return fputs ("&quot", fp);
   return fputc (c, fp);
   }


//#326 this code assumes and ident in link2 is also in link1
//This may not be the case for defines and typedefs


void WriteWithLinks (FILE *fpOut,   PSZ pszSrc, 
                     PLNKS plnks1,  PLNKS plnks2, 
                     PSZ pszInFile, PSZ pszOutFile)
   {
   PLNK plnk, plnkHold, plnkAlt;
   UINT cPrev;
   BOOL bWrote;

   /*--- see if there is any NAME records to write ---*/
   plnkHold = MaybeWriteNAME (fpOut, plnks1, pszInFile, pszOutFile);

   /*--- and HREF records to write ---*/
   cPrev = 0;
   while (*pszSrc)
      {
      bWrote = FALSE;
      if (isalnum (cPrev) || cPrev == '_')
         {
         fputHTMLc (cPrev = *pszSrc++, fpOut);
         continue;
         }

      if (!(plnk = FindTNode (plnks1->plnkT, pszSrc)))
         {
         if (plnk = FindTNode (plnks2->plnkT, pszSrc))
            {
            fprintf (fpOut, plnk->pszHREF);
            pszSrc += plnk->uSrcLen;
            cPrev  = 0;
            bWrote = TRUE;
            }
         else
            {
            fputHTMLc (cPrev = *pszSrc++, fpOut);
            }
         continue;
         }

      /*--- we now know we have a valid, isolated link ---*/
      if (plnk != plnkHold)
         {
         fprintf (fpOut, plnk->pszHREF);
         pszSrc += plnk->uSrcLen;
         cPrev  = 0;
         bWrote = TRUE;
         }

      /*--- link C define to H file -or- H declare to C file ---*/
      else if (plnkAlt = FindTNode (plnks2->plnkT, pszSrc))
         {
         fprintf (fpOut, plnkAlt->pszHREF);
         pszSrc += plnkAlt->uSrcLen;
         cPrev  = 0;
         bWrote = TRUE;
         }
      else
         {
         fputHTMLc (cPrev = *pszSrc++, fpOut);
         }
      }
   }


void WriteBody (FILE *fpOut,    FILE *fpIn, 
                PLNKS plnks1,   PLNKS plnks2, 
                PSZ  pszInFile, PSZ pszOutFile)
   {
   FilSetLine (0);
   fprintf (fpOut, "<A NAME=\"%s\"></A>", pszInFile);
   while (FilReadLine (fpIn, szBUFF, "", sizeof (szBUFF)) != (UINT)-1)
      {
      WriteWithLinks (fpOut, szBUFF, plnks1, plnks2, pszInFile, pszOutFile);
      fprintf (fpOut, "\n");
      }
   }

void ProcessFile (PFIL pFil, 
                  FILE *fpIn,   FILE *fpOut, 
                  PLNKS plnks1, PLNKS plnks2)
   {
   printf ("Processing File %s to %s ...", pFil->pszOld, pFil->pszNew);
   WriteBlock (fpOut, pszPRE, pFil);
   WriteBody  (fpOut, fpIn, plnks1, plnks2, pFil->pszOld, pFil->pszNew);
   WriteBlock (fpOut, pszPOST, pFil);
   printf ("\n");
   }


UINT ProcessFiles (PFIL pFil, PLNKS plnks1, PLNKS plnks2)
   {
   FILE  *fpIn, *fpOut;
   UINT  uCount;

   if (!pFil)
      return 0;

   uCount = ProcessFiles (pFil->left, plnks1, plnks2);

   if (!(fpIn = fopen (pFil->pszOld, "rt")))
      Error ("Cannot open input file %s", pFil->pszOld);
   if (!(fpOut = fopen (pFil->pszNew, "wt")))
      Error ("Cannot open output file %s", pFil->pszNew);

   ProcessFile (pFil, fpIn, fpOut, plnks1, plnks2);
   uCount++;

   fclose (fpIn);
   fclose (fpOut);

   uCount += ProcessFiles (pFil->right, plnks1, plnks2);

   return uCount;
   }


int _cdecl main (int argc, char *argv[])
   {
   UINT i, uCFiles, uHFiles;
   PFIL pFilC, pFilH;

   if (ArgBuildBlk ("*^Cfg% *^Var% *^CLinks% *^HLinks% *^Dump *^NoCase ?"))
      Error ("%s", ArgGetErr ());

   if (ArgFillBlk (argv))
      Error ("%s", ArgGetErr ());

   if (ArgFillBlk2 (getenv ("CPAGES")))
      return Error ("env cpages ERROR: %s", ArgGetErr ());

   if (ArgIs ("?"))
      Usage ();

   bCASE = !ArgIs ("NoCase");
   LoadCfg ("cpages.cfg", ArgGet ("Cfg", 0), argv[0]);
   cSORT = 'n';

   /*--- links files ---*/
   lnksC.plnkT = lnksC.plnkN = NULL;
   lnksH.plnkT = lnksH.plnkN = NULL;

   for (i=0; i < ArgIs ("CLinks"); i++)
      ReadAutolinkFile (&lnksC, ArgGet ("CLinks", i), "C");
   if (!i)
      ReadAutolinkFile (&lnksC, "C.LNK", "C");

   for (i=0; i < ArgIs ("HLinks"); i++)
      ReadAutolinkFile (&lnksH, ArgGet ("HLinks", i), "H");
   if (!i)
      ReadAutolinkFile (&lnksH, "H.LNK", "H");

   if (ArgIs ("Dump"))
      {
      DumpLinks (lnksC.plnkT);
      DumpLinks (lnksH.plnkT);
      exit (0);
      }

   pFilC = AddFiles ("*.c", NULL);
   MakeNames (pFilC, pFilC, "");

   pFilH = AddFiles ("*.h", NULL);
   MakeNames (pFilH, pFilH, "");

   uCFiles = ProcessFiles (pFilC, &lnksC, &lnksH);
   uHFiles = ProcessFiles (pFilH, &lnksH, &lnksC);

   printf ("%u C Files and %u H Files processed", uCFiles, uHFiles);
   return 0;
   }


